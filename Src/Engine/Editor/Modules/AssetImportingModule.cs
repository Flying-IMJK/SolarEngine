using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace SE.Editor
{

    /// <summary>
    /// Imports assets and other resources to the project. Provides per asset import settings editing.
    /// </summary>
    public sealed class AssetImportingModule : EditorModule
    {
        // Amount of requests done/total used to calculate importing progress

        private int m_ImportBatchDone;
        private int m_ImportBatchSize;

        // Firstly service is collecting import requests and then performs actual importing in the background.

        private readonly Queue<IFileEntryAction> m_ImportingQueue = new Queue<IFileEntryAction>();
        private readonly List<Request> m_Requests = new List<Request>();

        private long m_WorkerEndFlag;
        private Thread m_WorkerThread;

        /// <summary>
        /// Gets a value indicating whether this instance is importing assets.
        /// </summary>
        public bool IsImporting => m_ImportBatchSize > 0;

        /// <summary>
        /// Gets the importing assets progress.
        /// </summary>
        public float ImportingProgress => m_ImportBatchSize > 0 ? (float)m_ImportBatchDone / m_ImportBatchSize : 1.0f;

        /// <summary>
        /// Gets the amount of files done in the current import batch.
        /// </summary>
        public float ImportBatchDone => m_ImportBatchDone;

        /// <summary>
        /// Gets the size of the current import batch (imported files + files to import left).
        /// </summary>
        public int ImportBatchSize => m_ImportBatchSize;

        /// <summary>
        /// Occurs when assets importing starts.
        /// </summary>
        public event Action ImportingQueueBegin;

        /// <summary>
        /// Occurs when file is being imported. Can be called on non-main thread.
        /// </summary>
        public event Action<IFileEntryAction> ImportFileBegin;

        /// <summary>
        /// Import file end delegate.
        /// </summary>
        /// <param name="entry">The imported file entry.</param>
        /// <param name="failed">if set to <c>true</c> if importing failed, otherwise false.</param>
        public delegate void ImportFileEndDelegate(IFileEntryAction entry, bool failed);

        /// <summary>
        /// Occurs when file importing end. Can be called on non-main thread.
        /// </summary>
        public event ImportFileEndDelegate ImportFileEnd;

        /// <summary>
        /// Occurs when assets importing ends. Can be called on non-main thread.
        /// </summary>
        public event Action ImportingQueueEnd;

        /// <inheritdoc />
        internal AssetImportingModule(Editor editor) : base(editor)
        {
        }

        /// <summary>
        /// Creates the specified file entry (can show create settings dialog if needed).
        /// </summary>
        /// <param name="entry">The entry.</param>
        public void Create(CreateFileEntry entry)
        {
            if (entry.HasSettings)
            {
                // Use settings dialog
                var dialog = new CreateFilesDialog(entry);
                dialog.Show(Editor.MainWindow);
            }
            else
            {
                // Use direct creation
                LetThemBeCreatedxD(entry);
            }
        }

        /// <summary>
        /// Shows the dialog for selecting files to import.
        /// </summary>
        /// <param name="targetLocation">The target location.</param>
        public void ShowImportFileDialog(ContentFolder targetLocation)
        {
            // Ask user to select files to import
            if (!FileSystem.ShowOpenFileDialog(Editor.MainWindow, null, "All files (*.*)\0*.*\0", true, "Select files to import", out var files))
            {
                return;
            }
            if (files != null && files.Length > 0)
            {
                Import(files, targetLocation);
            }
        }

        /// <summary>
        /// Reimports the specified <see cref="BinaryAssetItem"/> item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="settings">The import settings to override.</param>
        /// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
        public void Reimport(BinaryAssetItem item, object settings = null, bool skipSettingsDialog = false)
        {
            if (item != null && !item.GetImportPath(out string importPath))
            {
                if (GetReimportPath(item.ShortName, ref importPath, skipSettingsDialog))
                {
                    return;
                }
                Import(importPath, item.Path, true, skipSettingsDialog, settings);
            }
        }

        internal bool GetReimportPath(string contextName, ref string importPath, bool skipSettingsDialog = false)
        {
            // Check if input file is missing
            if (!System.IO.File.Exists(importPath))
            {
                Debug.LogWarning(string.Format("Cannot reimport asset \'{0}\'. File \'{1}\' does not exist.", contextName, importPath));
                if (skipSettingsDialog)
                {
                    return true;
                }

                // Ask user to select new file location
                var title = string.Format("Please find missing \'{0}\' file for asset \'{1}\'", importPath, contextName);
                if (!FileSystem.ShowOpenFileDialog(Editor.MainWindow, null, "All files (*.*)\0*.*\0", false, title, out var files))
                {
                    return true;
                }
                if (files != null && files.Length > 0)
                {
                    importPath = files[0];
                }

                // Validate file path again
                if (!File.Exists(importPath))
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Imports the specified files.
        /// </summary>
        /// <param name="files">The files.</param>
        /// <param name="targetLocation">The target location.</param>
        /// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
        public void Import(IEnumerable<string> files, ContentFolder targetLocation, bool skipSettingsDialog = false)
        {
            if (targetLocation == null)
            {
                throw new ArgumentNullException();
            }
            if (files == null)
            {
                return;
            }

            lock (m_Requests)
            {
                bool skipDialog = skipSettingsDialog;
                foreach (var file in files)
                {
                    Import(file, targetLocation, skipSettingsDialog, null, ref skipDialog);
                }
            }
        }

        /// <summary>
        /// Imports the specified file.
        /// </summary>
        /// <param name="file">The file.</param>
        /// <param name="targetLocation">The target location.</param>
        /// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
        /// <param name="settings">Import settings to override. Use null to skip this value.</param>
        public void Import(string file, ContentFolder targetLocation, bool skipSettingsDialog = false, object settings = null)
        {
            bool skipDialog = skipSettingsDialog;
            Import(file, targetLocation, skipSettingsDialog, settings, ref skipDialog);
        }

        private void Import(string inputPath, ContentFolder targetLocation, bool skipSettingsDialog, object settings, ref bool skipDialog)
        {
            if (targetLocation == null)
            {
                throw new ArgumentNullException();
            }

            var extension = System.IO.Path.GetExtension(inputPath) ?? string.Empty;

            // Check if given file extension is a binary asset (.flax files) and can be imported by the engine
            var outputExtension = AssetsImporting.CanImport(extension);
            bool isBuilt = !string.IsNullOrEmpty(outputExtension);
            if (isBuilt)
            {
                outputExtension = '.' + outputExtension;

                if (!targetLocation.CanHaveAssets)
                {
                    // Error
                    Debug.LogWarning(string.Format("Cannot import \'{0}\' to \'{1}\'. The target directory cannot have assets.", inputPath, targetLocation.Node.GetPath()));
                    if (!skipDialog)
                    {
                        skipDialog = true;
                        MessageBox.Show("Target location cannot have assets. Use Content folder for your game assets.", "Cannot import assets", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    return;
                }
            }
            else
            {
                // Preserve file extension (will copy file to the import location)
                outputExtension = extension;

                // Check if can place source files here
                if (/*!targetLocation.CanHaveScripts && */(extension == ".cs" || extension == ".cpp" || extension == ".h" || extension == ".c" || extension == ".hpp"))
                {
                    // Error
                    Debug.LogWarning(string.Format("Cannot import \'{0}\' to \'{1}\'. The target directory cannot have scripts.", inputPath, targetLocation.Node.GetPath()));
                    if (!skipDialog)
                    {
                        skipDialog = true;
                        MessageBox.Show("Target location cannot have scripts. Use Source folder for your game source code.", "Cannot import assets", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    return;
                }
            }

            var shortName = System.IO.Path.GetFileNameWithoutExtension(inputPath);
            var outputPath = System.IO.Path.Combine(targetLocation.Path, shortName + outputExtension);

            Import(inputPath, outputPath, isBuilt, skipSettingsDialog, settings);
        }

        /// <summary>
        /// Imports the specified file to the target destination.
        /// Actual importing is done later after gathering settings from the user via <see cref="ImportFilesDialog"/>.
        /// </summary>
        /// <param name="inputPath">The input path.</param>
        /// <param name="outputPath">The output path.</param>
        /// <param name="isInBuilt">True if use in-built importer (engine backend).</param>
        /// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
        /// <param name="settings">Import settings to override. Use null to skip this value.</param>
        private void Import(string inputPath, string outputPath, bool isInBuilt, bool skipSettingsDialog = false, object settings = null)
        {
            inputPath = FileSystem.NormalizePath(inputPath);
            outputPath = FileSystem.NormalizePath(outputPath);
            lock (m_Requests)
            {
                m_Requests.Add(new Request
                {
                    InputPath = inputPath,
                    OutputPath = outputPath,
                    IsInBuilt = isInBuilt,
                    SkipSettingsDialog = skipSettingsDialog,
                    Settings = settings,
                });
            }
        }

        private void WorkerMain()
        {
            IFileEntryAction entry;
            bool wasLastTickWorking = false;

            while (Interlocked.Read(ref m_WorkerEndFlag) == 0)
            {
                // Try to get entry to process
                lock (m_Requests)
                {
                    if (m_ImportingQueue.Count > 0)
                    {
                        entry = m_ImportingQueue.Dequeue();
                    }
                    else
                    {
                        entry = null;
                    }
                }

                // Check if has any no job
                bool inThisTickWork = entry != null;
                if (inThisTickWork)
                {
                    // Check if begin importing
                    if (!wasLastTickWorking)
                    {
                        m_ImportBatchDone = 0;
                        ImportingQueueBegin?.Invoke();
                    }

                    // Import file
                    bool failed = true;
                    try
                    {
                        ImportFileBegin?.Invoke(entry);
                        failed = entry.Execute();
                    }
                    catch (Exception ex)
                    {
                        Debug.LogWarning(ex);
                    }
                    finally
                    {
                        if (failed)
                        {
                            Debug.LogWarning("Failed to import " + entry.SourceUrl + " to " + entry.ResultUrl);
                        }

                        m_ImportBatchDone++;
                        Profiler.BeginEvent("ImportFileEnd");
                        ImportFileEnd?.Invoke(entry, failed);
                        Profiler.EndEvent();
                    }
                }
                else
                {
                    // Check if end importing
                    if (wasLastTickWorking)
                    {
                        m_ImportBatchDone = m_ImportBatchSize = 0;
                        ImportingQueueEnd?.Invoke();
                    }

                    // Wait some time
                    Thread.Sleep(100);
                }

                wasLastTickWorking = inThisTickWork;
            }
        }

        internal void LetThemBeImportedxD(List<ImportFileEntry> entries)
        {
            int count = entries.Count;
            if (count > 0)
            {
                lock (m_Requests)
                {
                    m_ImportBatchSize += count;
                    for (int i = 0; i < count; i++)
                    {
                        m_ImportingQueue.Enqueue(entries[i]);
                    }
                }

                StartWorker();
            }
        }

        internal void LetThemBeCreatedxD(CreateFileEntry entry)
        {
            lock (m_Requests)
            {
                m_ImportBatchSize += 1;
                m_ImportingQueue.Enqueue(entry);
            }

            StartWorker();
        }

        private void StartWorker()
        { 
            if (m_WorkerThread != null)
                return;

            m_WorkerEndFlag = 0;
            m_WorkerThread = new Thread(WorkerMain)
            {
                Name = "Content Importer",
                Priority = ThreadPriority.Highest
            };
            m_WorkerThread.Start();
        }

        private void EndWorker()
        {
            if (m_WorkerThread == null)
                return;

            Interlocked.Increment(ref m_WorkerEndFlag);
            Thread.Sleep(0);

            m_WorkerThread.Join(1000);
#if !USE_NETCORE
            m_WorkerThread.Abort(); // Deprecated in .NET 7
#endif
            m_WorkerThread = null;
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            ImportFileEntry.RegisterDefaultTypes();
            //ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
        }

        private void OnScriptsReloadBegin()
        {
            // Remove import file types from scripting assemblies
            List<string> removeFileTypes = new List<string>();
            foreach (var pair in ImportFileEntry.FileTypes)
            {
                if (pair.Value.Method.IsCollectible || (pair.Value.Target != null && pair.Value.Target.GetType().IsCollectible))
                {
                    removeFileTypes.Add(pair.Key);
                }
            }
            foreach (var fileType in removeFileTypes)
            {
                ImportFileEntry.FileTypes.Remove(fileType);
            }
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            // Check if has no requests to process
            if (m_Requests.Count == 0)
            {
                return;
            }

            lock (m_Requests)
            {
                try
                {
                    // Get entries
                    List<ImportFileEntry> entries = new List<ImportFileEntry>(m_Requests.Count);
                    bool needSettingsDialog = false;
                    for (int i = 0; i < m_Requests.Count; i++)
                    {
                        var request = m_Requests[i];
                        var entry = ImportFileEntry.CreateEntry(ref request);
                        if (entry != null)
                        {
                            if (request.Settings != null && entry.TryOverrideSettings(request.Settings))
                            {
                                // Use overridden settings
                            }
                            else if (!request.SkipSettingsDialog)
                            {
                                needSettingsDialog |= entry.HasSettings;
                            }

                            entries.Add(entry);
                        }
                    }
                    m_Requests.Clear();

                    // Check if need to show importing dialog or can just pass requests
                    if (needSettingsDialog)
                    {
                        var dialog = new ImportFilesDialog(entries);
                        dialog.Show(Editor.MainWindow);
                        dialog.Focus();
                    }
                    else
                    {
                        LetThemBeImportedxD(entries);
                    }
                }
                catch (Exception ex)
                {
                    // Error
                    Debug.LogError(ex);
                    Debug.LogError("Failed to process files import request.");
                }
            }
        }


        /// <inheritdoc />
        public override void OnDispose()
        {
            //ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            EndWorker();
        }
    }
}
