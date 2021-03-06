unit Main;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, ExtCtrls, StdCtrls, XPMan, OpenFolderDialog;

type
  TMainForm = class(TForm)
    BottomBevel: TBevel;
    CancelButton: TButton;
    NextButton: TButton;
    TopImage: TImage;
    TopBevel: TBevel;
    InfoLabel1: TLabel;
    WolfpackIcon: TImage;
    XPManifest: TXPManifest;
    GroupBox1: TGroupBox;
    WolfpackPath: TEdit;
    BrowseButton: TButton;
    InfoLabel2: TLabel;
    OpenFolderDialog: TOpenFolderDialog;
    procedure CancelButtonClick(Sender: TObject);
    procedure BrowseButtonClick(Sender: TObject);
    procedure OpenFolderDialogSelectionChange(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure NextButtonClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  MainForm: TMainForm;

implementation

{$R *.dfm}

uses Registry, WpConfiguration;

{
  Close the wizard.
}
procedure TMainForm.CancelButtonClick(Sender: TObject);
begin
  Close;
end;

{
  Let the user select a directory.
}
procedure TMainForm.BrowseButtonClick(Sender: TObject);
begin
  if OpenFolderDialog.Execute then
    WolfpackPath.Text := OpenFolderDialog.Folder;
end;

{
  Show the user if the directory he selected is valid or not.
}
procedure TMainForm.OpenFolderDialogSelectionChange(Sender: TObject);
begin
  if FileExists(IncludeTrailingPathDelimiter(OpenFolderDialog.Folder) + 'wolfpack.xml') then
    OpenFolderDialog.StatusText := 'This is a valid Wolfpack installation directory.'
  else
    OpenFolderDialog.StatusText := 'This is not a valid Wolfpack installation directory.';
end;

(*
  Query the Wolfpack installation directory from the uninstallation database
  @ HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{44306150-2736-4554-ACD5-957D5F12604B}}_is1
*)
procedure TMainForm.FormCreate(Sender: TObject);
var
  Registry: TRegistry;
begin
  Registry := TRegistry.Create;
  Registry.RootKey := HKEY_LOCAL_MACHINE;

  // Open the Uninstall Key.
  if not Registry.OpenKeyReadOnly('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{44306150-2736-4554-ACD5-957D5F12604B}}_is1') then begin
    Registry.Free;
    exit;
  end;

  // Read Installation Directory
  WolfpackPath.Text := Registry.ReadString('Inno Setup: App Path');

  Registry.Free;
end;

procedure TMainForm.NextButtonClick(Sender: TObject);
var
  WpConfiguration: TWpConfiguration;
begin
  // Try to load the configuration file
  try
    WpConfiguration := TWpConfiguration.Create(IncludeTrailingPathDelimiter(WolfpackPath.Text) + 'wolfpack.xml');
    ShowMessage(WpConfiguration.ImportDefinitions.GetText);
    WpConfiguration.Free;
  except
    Application.MessageBox('An error occured while loading the wolfpack.xml file from your Wolfpack installation directory', 'Error', MB_OK+MB_ICONERROR);
  end;
end;

end.
