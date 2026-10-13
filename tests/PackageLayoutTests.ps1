param([Parameter(Mandatory=$true)][string]$Package)
$ErrorActionPreference='Stop'
$required=@('录制器\Piano Auto Sustain Recorder.exe','Cubase MIDI 插入\Piano Auto Sustain Cubase.dll','VST3 乐器\Piano Auto Sustain.vst3\Contents\x86_64-win\Piano Auto Sustain.vst3','Cubase 设置\README.md','说明与许可\系统兼容性.md','快速入门-中文.md','Quick Start-English.md','README.md','安装 Windows.ps1','卸载 Windows.ps1','SHA256SUMS.txt')
foreach($item in $required){if(-not(Test-Path -LiteralPath (Join-Path $Package $item))){throw "Missing: $item"}}
$readme=Get-Content -LiteralPath (Join-Path $Package 'README.md') -Raw
if($readme -notmatch '2\.2\.1'){throw 'README version missing'}
if($readme -notmatch '快速模式' -or $readme -notmatch '录制 CC64'){throw 'Two-mode guidance missing'}
if($readme -match 'BassPedalMidi|2\.0\.0'){throw 'Stale public name/version found'}
$installer=Get-Content -LiteralPath (Join-Path $Package '安装 Windows.ps1') -Raw
if($installer -notmatch 'CreateShortcut' -or $installer -notmatch 'Start-Process'){throw 'Installer does not create shortcut and launch Recorder'}
if($installer -notmatch 'Get-CimInstance Win32_Process'){throw 'Installer cannot stop an existing Recorder instance'}
$lines=Get-Content -LiteralPath (Join-Path $Package 'SHA256SUMS.txt')
if($lines.Count -lt 10){throw 'Checksum list incomplete'}
foreach($line in $lines){$parts=$line -split '  ',2; $path=Join-Path $Package ($parts[1].Replace('/','\')); if(-not(Test-Path -LiteralPath $path)){throw "Checksum path missing: $($parts[1])"}; if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower() -ne $parts[0]){throw "Checksum mismatch: $($parts[1])"}}
'Package layout checks passed.'
