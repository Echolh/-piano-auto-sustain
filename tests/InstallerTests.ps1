param([Parameter(Mandatory=$true)][string]$Package)
$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot ('.installer-test-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
try {
 $sentinel=Join-Path $root 'Unrelated.vst3';Set-Content -LiteralPath $sentinel 'keep'
 & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Package 'Install-VST3.ps1') -DestinationRoot $root -Quiet
 if($LASTEXITCODE -ne 0){throw 'Install failed'}
 $binary=Join-Path $root 'Piano Auto Sustain.vst3/Contents/x86_64-win/Piano Auto Sustain.vst3'
 if(!(Test-Path -LiteralPath $binary)){throw 'Missing binary'}
 & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Package 'Install-VST3.ps1') -DestinationRoot $root -Quiet
 if($LASTEXITCODE -ne 0){throw 'Upgrade failed'}
 & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Package 'Install-VST3.ps1') -DestinationRoot $root -Action Uninstall -Quiet
 if($LASTEXITCODE -ne 0 -or (Test-Path -LiteralPath $binary)){throw 'Uninstall failed'}
 if((Get-Content -LiteralPath $sentinel) -ne 'keep'){throw 'Unrelated plugin changed'}
 Write-Output 'PASS: install, upgrade, uninstall, unrelated-plugin preservation'
} finally {
 $resolved=[IO.Path]::GetFullPath($root);$parent=[IO.Path]::GetFullPath($PSScriptRoot)
 if([IO.Path]::GetDirectoryName($resolved) -ne $parent){throw 'Unsafe test cleanup'}
 Remove-Item -LiteralPath $resolved -Recurse -Force
}
