param([ValidateSet('Install','Uninstall')][string]$Action='Install',[string]$DestinationRoot='',[switch]$Quiet)
$ErrorActionPreference='Stop'
$customDestination=![string]::IsNullOrWhiteSpace($DestinationRoot)
if(!$customDestination){$DestinationRoot=Join-Path $env:CommonProgramFiles 'VST3'}
$root=[IO.Path]::GetFullPath($DestinationRoot).TrimEnd('\','/')
$target=Join-Path $root 'Piano Auto Sustain.vst3'
if([IO.Path]::GetDirectoryName($target) -ne $root){throw 'Invalid target'}
function NoLinks([string]$path,[bool]$recursive=$false){if(Test-Path -LiteralPath $path){$all=@(Get-Item -LiteralPath $path);if($recursive){$all+=@(Get-ChildItem -LiteralPath $path -Force -Recurse)};foreach($item in $all){if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Refusing reparse-point installation path'}}}}
function Owns([string]$path){
 $binary=Join-Path $path 'Contents/x86_64-win/Piano Auto Sustain.vst3'
 if(!(Test-Path -LiteralPath $binary)){return $false}
 return (Get-Item -LiteralPath $binary).VersionInfo.ProductName -eq 'Piano Auto Sustain'
}
function CheckFiles([string]$path){
 $manifest=Join-Path $PSScriptRoot 'SHA256SUMS.txt'
 if(!(Test-Path -LiteralPath $manifest)){throw 'Package checksum manifest missing'}
 $checked=0
 foreach($line in Get-Content -LiteralPath $manifest){
  if($line -match '^([0-9a-fA-F]{64})  Piano Auto Sustain\.vst3[/\\](.+)$'){
   $hash=$Matches[1];$relative=$Matches[2];$file=[IO.Path]::GetFullPath((Join-Path $path $relative))
   if(!$file.StartsWith([IO.Path]::GetFullPath($path)+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Invalid checksum path'}
   if((Get-FileHash -LiteralPath $file).Hash -ne $hash){throw "Checksum mismatch: $relative"};$checked++
  }
 }
 if($checked -lt 1){throw 'No plugin files in manifest'}
}
if(!$customDestination){
 $admin=([Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
 if(!$admin){$args='-NoProfile -ExecutionPolicy Bypass -File "'+$PSCommandPath+'" -Action '+$Action;if($Quiet){$args+=' -Quiet'};$p=Start-Process powershell.exe -ArgumentList $args -Verb RunAs -WindowStyle Hidden -Wait -PassThru;exit $p.ExitCode}
}
try {
 NoLinks $root;NoLinks $target $true
 if($Action -eq 'Uninstall'){
  if(!(Test-Path -LiteralPath $target)){Write-Output 'Already uninstalled.';exit 0}
  if(!(Owns $target)){throw 'Target does not identify as Piano Auto Sustain; refusing removal'}
  Remove-Item -LiteralPath $target -Recurse -Force
  Write-Output 'Piano Auto Sustain VST3 uninstalled. Other plugins were not changed.'
 } else {
  $source=Join-Path $PSScriptRoot 'Piano Auto Sustain.vst3';NoLinks $source $true
  if(!(Owns $source)){throw 'Package plugin identity mismatch'}
  CheckFiles $source
  if((Test-Path -LiteralPath $target) -and !(Owns $target)){throw 'Existing target identity mismatch; not overwritten'}
  New-Item -ItemType Directory -Path $root -Force | Out-Null
  $stage=Join-Path $root ('.pas-stage-'+[guid]::NewGuid().ToString('N'))
  $backup=Join-Path $root ('.pas-backup-'+[guid]::NewGuid().ToString('N'))
  $moved=$false
  try {
   Copy-Item -LiteralPath $source -Destination $stage -Recurse
   CheckFiles $stage
   if(Test-Path -LiteralPath $target){Move-Item -LiteralPath $target -Destination $backup;$moved=$true}
   try {Move-Item -LiteralPath $stage -Destination $target;CheckFiles $target}
   catch {if(Test-Path -LiteralPath $target){Remove-Item -LiteralPath $target -Recurse -Force};if($moved){Move-Item -LiteralPath $backup -Destination $target;$moved=$false};throw}
   if($moved){Remove-Item -LiteralPath $backup -Recurse -Force;$moved=$false}
  } finally {if(Test-Path -LiteralPath $stage){Remove-Item -LiteralPath $stage -Recurse -Force}}
  Write-Output "Installed: $target"
 }
 if(!$Quiet){Write-Output 'Restart your DAW and rescan plugins. / 请重启宿主并扫描插件。'}
} catch {Write-Error ('Installation not completed. Close the DAW and retry. '+$_.Exception.Message);exit 1}
