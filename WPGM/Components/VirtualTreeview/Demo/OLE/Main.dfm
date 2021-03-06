�
 TMAINFORM 0  TPF0	TMainFormMainFormLeft� TopfWidthHeightZCaption,Demo for drag'n drop and clipboard transfersColor	clBtnFaceFont.CharsetANSI_CHARSET
Font.ColorclWindowTextFont.Height�	Font.NameArial
Font.Style OldCreateOrderVisible	OnCreate
FormCreate
DesignSize? PixelsPerInch`
TextHeight TLabelLabel1Left
Top`Width� HeightCaption1Tree 1 uses OLE when initiating a drag operation.Font.CharsetANSI_CHARSET
Font.ColorclWindowTextFont.Height�	Font.NameArial
Font.Style 
ParentFontTransparent	  TLabelLabel2LefthTopPWidthQHeight!AutoSizeCaption�Tree 2 uses VCL when initiating a drag operation. It also uses manual drag mode. Only marked lines are allowed to start a drag operation.Font.CharsetANSI_CHARSET
Font.ColorclWindowTextFont.Height�	Font.NameArial
Font.Style 
ParentFontTransparent	WordWrap	  TPanelPanel3Left Top WidthHeightEAlignalTopColorclWhiteTabOrder  TLabelLabel6Left$TopWidthHeight*AutoSizeCaptionPThis demo shows how to cope with OLE drag'n drop as well as cut, copy and paste.Font.CharsetANSI_CHARSET
Font.ColorclWindowTextFont.Height�	Font.NameArial
Font.StylefsBold 
ParentFontTransparent	WordWrap	   TButtonButton1Left�TopWidthKHeightAnchorsakRightakBottom CaptionCloseTabOrderOnClickButton1Click  TButtonButton3Left�TopPWidthKHeightAnchorsakTopakRight CaptionTree font...ParentShowHintShowHintTabOrderOnClickButton3Click  TVirtualStringTreeTree2LeftlToptWidthJHeight� 
BevelInnerbvNoneBorderStylebsSingleClipboardFormats.Strings
Plain textUnicode textVirtual Tree Data Colors.BorderColorclWindowTextColors.HotColorclBlackDefaultNodeHeightDragOperationsdoCopydoMovedoLink DragTypedtVCL	DragWidth^	EditDelay�Font.CharsetANSI_CHARSET
Font.ColorclWindowTextFont.Height�	Font.NameLucida Sans Unicode
Font.Style Header.AutoSizeIndex Header.Columns Header.Font.CharsetDEFAULT_CHARSETHeader.Font.ColorclWindowTextHeader.Font.Height�Header.Font.NameMS Sans SerifHeader.Font.Style Header.MainColumn�Header.OptionshoColumnResizehoDrag Header.StylehsThickButtonsHintAnimationhatSystemDefaultHintMode	hmTooltipIncrementalSearchDirection	sdForward
ParentFontParentShowHintShowHint	TabOrderTreeOptions.AnimationOptionstoAnimatedToggle TreeOptions.AutoOptionstoAutoDropExpandtoAutoScrollOnExpandtoAutoTristateTrackingtoAutoHideButtons TreeOptions.MiscOptionstoAcceptOLEDroptoInitOnSavetoToggleOnDblClicktoWheelPanning TreeOptions.SelectionOptionstoMultiSelecttoCenterScrollIntoView OnBeforeItemEraseTree2BeforeItemEraseOnDragAllowedTree2DragAllowed
OnDragOverTreeDragOver
OnDragDropTreeDragDrop	OnGetTextTree1GetText
OnInitNodeTreeInitNode	OnNewTextTree1NewTextColumns   TVirtualStringTreeTree1Left
ToptWidthJHeight� BorderStylebsSingleClipboardFormats.StringsCSVHTML Format
Plain textRich Text Format Rich Text Format Without ObjectsUnicode textVirtual Tree Data Colors.BorderColorclWindowTextColors.HotColorclBlackDefaultNodeHeightDragModedmAutomaticDragOperationsdoCopydoMovedoLink 	DragWidth^	EditDelay�Font.CharsetANSI_CHARSET
Font.ColorclWindowTextFont.Height�	Font.NameVerdana
Font.Style Header.AutoSizeIndex Header.Columns Header.Font.CharsetDEFAULT_CHARSETHeader.Font.ColorclWindowTextHeader.Font.Height�Header.Font.NameMS Sans SerifHeader.Font.Style Header.MainColumn�Header.OptionshoColumnResizehoDrag Header.StylehsThickButtonsHintAnimationhatSystemDefaultHintMode	hmTooltipIncrementalSearchDirection	sdForward
ParentFontParentShowHintShowHint	TabOrderTreeOptions.AnimationOptionstoAnimatedToggle TreeOptions.AutoOptionstoAutoDropExpandtoAutoScrolltoAutoScrollOnExpandtoAutoTristateTrackingtoAutoHideButtons TreeOptions.MiscOptionstoAcceptOLEDroptoInitOnSavetoToggleOnDblClicktoWheelPanning TreeOptions.SelectionOptionstoMultiSelecttoCenterScrollIntoView 
OnDragOverTreeDragOver
OnDragDropTreeDragDrop	OnGetTextTree1GetText
OnInitNodeTreeInitNode	OnNewTextTree1NewTextColumns   TPageControlPageControl1LeftTop4Width�Height� 
ActivePageLogTabSheetAnchorsakLeftakTopakBottom TabIndexTabOrder 	TTabSheetRichTextTabSheetCaption	Rich text
ImageIndex
DesignSize��   TLabelLabel3LeftTopWidth�HeightCaptionWYou can use the rich edit control as source and as target. It initiates OLE drag' drop.Transparent	  	TRichEdit	RichEdit1LeftTop Width�Height� AnchorsakLeftakTopakRightakBottom HideSelectionLines.Strings	RichEdit1 
ScrollBarsssBothTabOrder WordWrap   	TTabSheetLogTabSheetCaptionDrag'n drop operation log
DesignSize��   TLabelLabel7LeftTopWidth�Height)AutoSizeCaptionzThe log below shows textual representations of the operation carried out. You can also use the control as VCL drag source.Transparent	WordWrap	  TListBox
LogListBoxLeftTop8Width�Height� Hint/Use the list box to initiate a VCL drag'n drop.AnchorsakLeftakTopakRightakBottom DragModedmAutomatic
ItemHeightParentShowHintShowHint	TabOrder   TButtonButton2LeftNTop
WidthKHeightAnchorsakTopakRight Caption	Clear logTabOrderOnClickButton2Click   	TTabSheet	TabSheet2Caption	More info
ImageIndex TLabelLabel4LeftTop4Width�Height%AutoSizeCaption�For drag'n drop however it can (mainly for compatibility) either use OLE or VCL for drag operations. Since both approaches are incompatible and cannot be used together only one of them can be actived at a time.Transparent	WordWrap	  TLabelLabel5LeftTop^Width�HeightAutoSizeCaptionnThis, though, applies only for the originator of a drag operation. The receiver can handle both simultanously.Transparent	WordWrap	  TLabelLabel9LeftTopWidth}Height!AutoSizeCaption�Virtual Treeview always uses OLE for clipboard operations. Windows ensures that  an IDataObject is always available, even if an application used the clipboard in the old way.Transparent	WordWrap	   	TTabSheet	TabSheet1CaptionTips
ImageIndex TLabelLabel8LeftTopWidth�Height%AutoSizeCaptionrTry drag'n drop and clipboard operations also together with other applications like Word or the Internet Explorer.Transparent	WordWrap	  TLabelLabel10LeftTop0Width�Height%AutoSizeCaption�Also quite interesting is to start more than one instance of this demo and drag data between these instances. This works however only for OLE drag' drop.Transparent	WordWrap	    TActionListActionList1Left�Top�  TAction	CutActionCaptionCutShortCutX@	OnExecuteCutActionExecute  TAction
CopyActionCaptionCopyShortCutC@	OnExecuteCopyActionExecute  TActionPasteActionCaptionPasteShortCutV@	OnExecutePasteActionExecute   TFontDialog
FontDialogFont.CharsetDEFAULT_CHARSET
Font.ColorclWindowTextFont.Height�	Font.NameMS Sans Serif
Font.Style MinFontSize MaxFontSize Left�Top�    