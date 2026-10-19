

using System;
using SE.Editor.GUI;
using SE.GUI;


namespace SE.Editor
{
    /// <summary>
    /// Dialog used to edit new file settings.
    /// </summary>
    /// <seealso cref="SE.Editor.GUI.Dialog" />
    public class CreateFilesDialog : Dialog
    {
        private CreateFileEntry m_Entry;
        //private CustomEditorPresenter _settingsEditor;

        /// <summary>
        /// Initializes a new instance of the <see cref="CreateFilesDialog"/> class.
        /// </summary>
        /// <param name="entry">The entry to edit it's settings.</param>
        public CreateFilesDialog(CreateFileEntry entry) : base("Create file settings")
        {
            m_Entry = entry ?? throw new ArgumentNullException();

            const float totalWidth = 520;
            const float editorHeight = 250;
            Width = totalWidth;

            // Header and help description
            Label headerLabel = new Label
            {
                Text = "Asset Options",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, 40),
                Parent = this,
                Font = new FontReference(Style.Current.FontTitle)
            };
            Label infoLabel = new Label
            {
                Text = "Specify options for creating new asset",
                HorizontalAlignment = TextAlignment.Near,
                Margin = new Margin(7),
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(10, -20, 45, 70),
                Parent = this
            };

            // Buttons
            const float buttonsWidth = 60;
            const float buttonsHeight = 24;
            const float buttonsMargin = 8;
            Button createButton = new Button
            {
                Text = "Create",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-buttonsWidth - buttonsMargin, buttonsWidth, -buttonsHeight - buttonsMargin,
                    buttonsHeight),
                Parent = this,
                Enabled = entry.CanBeCreated,
            };
            createButton.Clicked += OnSubmit;
            Button cancelButton = new Button
            {
                Text = "Cancel",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-buttonsWidth - buttonsMargin - buttonsWidth - buttonsMargin, buttonsWidth,
                    -buttonsHeight - buttonsMargin, buttonsHeight),
                Parent = this,
            };
            cancelButton.Clicked += OnCancel;

            // Panel for settings editor
            Panel panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(2, 2, infoLabel.Bottom + 2, editorHeight),
                Parent = this,
            };

            // Settings editor
            /*            _settingsEditor = new CustomEditorPresenter(null);
                        _settingsEditor.Panel.Parent = panel;*/

            m_DialogSize = new Float2(totalWidth, panel.Bottom);

            /*_settingsEditor.Select(_entry.Settings);
            _settingsEditor.Modified += () => createButton.Enabled = _entry.CanBeCreated;*/
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            Editor.Instance.Importing.LetThemBeCreatedxD(m_Entry);

            base.OnSubmit();
        }

        /// <inheritdoc />
        protected override void SetupWindowSettings(ref CreateWindowSettings settings)
        {
            base.SetupWindowSettings(ref settings);

            settings.MinimumSize = new Float2(300, 400);
            settings.HasSizingFrame = true;
        }
    }
}
