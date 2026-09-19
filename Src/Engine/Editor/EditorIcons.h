#pragma once
#include "Runtime/Render/2D/SpriteAtlas.h"

namespace SE
{
    class Texture;
}

namespace SE::Editor
{
    SE_CLASS(API(Static))
    class EditorIcons
    {
        SCRIPTING_TYPE_MIN(EditorIcons)
    public:
        // 12px
        SE_FIELD(API(ReadOnly)) static SpriteHandle DragBar12;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Search12;
        SE_FIELD(API(ReadOnly)) static SpriteHandle WindowDrag12;
        SE_FIELD(API(ReadOnly)) static SpriteHandle CheckBoxIntermediate12;
        SE_FIELD(API(ReadOnly)) static SpriteHandle ArrowRight12;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Settings12;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Cross16;
        SE_FIELD(API(ReadOnly)) static SpriteHandle CheckBoxTick12;
        SE_FIELD(API(ReadOnly)) static SpriteHandle ArrowDown12;

        // 32px
        SE_FIELD(API(ReadOnly)) static SpriteHandle Scalar32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Translate32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Rotate32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Scale32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Grid32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Flax32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle RotateSnap32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle ScaleSnap32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Globe32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle CamSpeed32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Link32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Add32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Left32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Right32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Up32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Down32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle FolderClosed32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle FolderOpen32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Folder32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle CameraFill32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Search32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Info32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Warning32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Error32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Bone32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle BoneFull32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle LogWindow32;

        // Visject
        SE_FIELD(API(ReadOnly)) static SpriteHandle VisjectBoxOpen32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle VisjectBoxClosed32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle VisjectArrowOpen32;
        SE_FIELD(API(ReadOnly)) static SpriteHandle VisjectArrowClosed32;

        // 64px
        SE_FIELD(API(ReadOnly)) static SpriteHandle Flax64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Save64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Play64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Stop64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Pause64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Skip64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Info64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Error64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Warning64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle AddFile64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle DeleteFile64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Import64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Left64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Right64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Up64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Down64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Undo64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Redo64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Translate64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Rotate64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Scale64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Refresh64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Shift64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Code64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Folder64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle CenterView64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Image64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Camera64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Docs64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Search64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Bone64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Link64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Build64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Add64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle ShipIt64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle SplineFree64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle SplineLinear64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle SplineAligned64;
        SE_FIELD(API(ReadOnly)) static SpriteHandle SplineSmoothIn64;

        // 96px
        SE_FIELD(API(ReadOnly)) static SpriteHandle Toolbox96;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Paint96;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Foliage96;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Terrain96;

        // 128px
        SE_FIELD(API(ReadOnly)) static SpriteHandle AndroidSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle PlaystationSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle InputSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle PhysicsSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle CSharpScript128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Folder128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle WindowsIcon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle LinuxIcon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle UWPSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle XBOXSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle LayersTagsSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle GraphicsSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle CPPScript128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Plugin128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle XBoxScarletIcon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle AssetShadow128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle WindowsSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle TimeSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle GameSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle VisualScript128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Document128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle XBoxOne128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle UWPStore128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle ColorWheel128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle LinuxSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle NavigationSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle AudioSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle BuildSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Scene128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle AndroidIcon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle PS4Icon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle PS5Icon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle MacOSIcon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle IOSIcon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle FlaxLogo128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle SwitchIcon128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle SwitchSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle LocalizationSettings128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle Json128;
        SE_FIELD(API(ReadOnly)) static SpriteHandle AppleSettings128;
    };

}
