[Setup]
AppName=ImageStereoMultiband
AppVersion=1.0.0
AppPublisher=Pedro Cuomo Ghio
AppComments=Plugin VST3 de procesamiento estéreo multibanda
DefaultGroupName=ImageStereoMultiband
OutputDir=.
OutputBaseFilename=ImageStereoMultiband_Installer
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=yes
DirExistsWarning=no
DefaultDirName={autopf64}\ImageStereoMultiband
DisableDirPage=yes

[Files]
; VST3 plugin — se instala como bundle en la carpeta común de VST3
Source: "Builds\VisualStudio2026\x64\Debug\VST3\ImageStereoMultiband.vst3\*"; DestDir: "{commoncf64}\VST3\ImageStereoMultiband.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: dirifempty; Name: "{commoncf64}\VST3\ImageStereoMultiband.vst3"
