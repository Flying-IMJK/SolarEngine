
using SE.GUI;

namespace SE.Editor
{

    /// <summary>
    /// Root tree node for the project workspace.
    /// </summary>
    public class ProjectTreeNode : ContentTreeNode
    {
        /// <summary>
        /// The project.
        /// </summary>
        public ProjectInfo Project;

        /// <summary>
        /// The project content directory.
        /// </summary>
        public MainContentTreeNode Content;

        /// <summary>
        /// The project source code directory.
        /// </summary>
        public MainContentTreeNode Source;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProjectTreeNode"/> class.
        /// </summary>
        public ProjectTreeNode(ProjectInfo project) : base(null, project.ProjectFolderPath)
        {
            Project = project;
            ProjectName = project.Name;
            ContentFolder folder = GetFolder();
            folder.FileName = project.Name;
            folder.ShortName = project.Name;
            Text = project.Name;
        }

        public string ProjectName { get; }

        /// <inheritdoc />
        public override string GetNavButtonLabel()
        {
            // return Project.Name;
            return ProjectName;
        }

        /// <inheritdoc />
        public override int CompareTo(Control other)
        {
            // Move the main game project to the top
            // if (Project.Name == Editor.Instance.Project.Name)
            // {
            //     return -1;
            // }
            return base.CompareTo(other);
        }

        /// <inheritdoc />
        protected override void BeginDragDrop()
        {
            // No drag for root nodes
        }
    }
}

