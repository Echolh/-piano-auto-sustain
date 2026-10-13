param([ValidateSet('info','runs','jobs','artifacts','logs','download','upload','annotations')][string]$Action='info',[string]$Id='',[string]$Output='')
$ErrorActionPreference='Stop'
$psi=[Diagnostics.ProcessStartInfo]::new('git','credential fill');$psi.UseShellExecute=$false;$psi.RedirectStandardInput=$true;$psi.RedirectStandardOutput=$true
$p=[Diagnostics.Process]::Start($psi);$p.StandardInput.Write("protocol=https`nhost=github.com`n`n");$p.StandardInput.Close();$raw=$p.StandardOutput.ReadToEnd();$p.WaitForExit()
$kv=@{};foreach($line in ($raw -split "`r?`n")){if($line -match '^([^=]+)=(.*)$'){$kv[$matches[1]]=$matches[2]}}
if(!$kv.password){throw 'GitHub credential unavailable'}
$headers=@{'User-Agent'='PianoAutoSustain-build';Authorization='Basic '+[Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($kv.username+':'+$kv.password))}
$base='https://api.github.com/repos/yuquny006-hash/amu-music-bridge'
function Api($path,$method='GET',$data=$null){$a=@{Uri="$base/$path";Headers=$headers;Method=$method;TimeoutSec=60;NoProxy=$true};if($null -ne $data){$a.ContentType='application/json';$a.Body=[Text.Encoding]::UTF8.GetBytes(($data|ConvertTo-Json -Depth 40 -Compress))};Invoke-RestMethod @a}
$branch='piano-auto-sustain-1.0-build'
switch($Action){
'annotations' {
 $job=Api "actions/jobs/$Id"
 $c=Invoke-RestMethod -Uri ($job.check_run_url+'/annotations') -Headers $headers -NoProxy
 $c | Select-Object message,title | ConvertTo-Json
}
'info' { $r=Invoke-RestMethod -Uri $base -Headers $headers -NoProxy; $r|Select-Object full_name,private,default_branch; (Api 'actions/runs?branch=amu-piano&per_page=1').workflow_runs|Select-Object id,status,conclusion,html_url }
'runs' {(Api "actions/runs?branch=$branch&per_page=3").workflow_runs|Select-Object id,head_sha,status,conclusion,html_url|ConvertTo-Json}
'jobs' {(Api "actions/runs/$Id/jobs").jobs|Select-Object id,name,status,conclusion,steps|ConvertTo-Json -Depth 8}
'artifacts' {(Api "actions/runs/$Id/artifacts").artifacts|Select-Object id,name,size_in_bytes|ConvertTo-Json}
'logs' {Invoke-WebRequest -Uri "$base/actions/jobs/$Id/logs" -Headers $headers -NoProxy -OutFile $Output;Get-Content -LiteralPath $Output | Select-String 'error:|error |FAILED|FAIL:|CMake Error' | Select-Object -Last 25}
'download' {Invoke-WebRequest -Uri "$base/actions/artifacts/$Id/zip" -Headers $headers -NoProxy -OutFile $Output;Write-Output "Downloaded artifact $Id"}
'upload' {
 $exists=$true
 try {$ref=Api "git/ref/heads/$branch"} catch {if([int]$_.Exception.Response.StatusCode -ne 404){throw};$exists=$false;$ref=Api 'git/ref/heads/amu-piano'}
 $parent=$ref.object.sha;$commit=Api "git/commits/$parent"
 $repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
 $files=@(Get-Item (Join-Path $repo 'CMakeLists.txt'))
 foreach($folder in @('Source','cmake','tests','scripts','docs','licenses')){$files+=Get-ChildItem -LiteralPath (Join-Path $repo $folder) -File -Recurse | Where-Object {$_.Extension -in @('.h','.cpp','.rc','.cmake','.py','.ps1','.sh','.md','.txt','.mid','.log')}}
 $tree=@()
 foreach($file in $files){
  $relative=$file.FullName.Substring($repo.Length+1).Replace('\','/')
  if($file.Extension -eq '.mid'){$blob=Api 'git/blobs' 'POST' @{content=[Convert]::ToBase64String([IO.File]::ReadAllBytes($file.FullName));encoding='base64'};$tree+=@{path="piano-auto-sustain/$relative";mode='100644';type='blob';sha=$blob.sha}}
  else {$tree+=@{path="piano-auto-sustain/$relative";mode=$(if($file.Extension -eq '.sh'){'100755'}else{'100644'});type='blob';content=[IO.File]::ReadAllText($file.FullName)}}
 }
 $tree+=@{path='.github/workflows/build-piano-auto-sustain.yml';mode='100644';type='blob';content=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'github-build-workflow.yml'))}
 $newTree=Api 'git/trees' 'POST' @{base_tree=$commit.tree.sha;tree=$tree}
 $newCommit=Api 'git/commits' 'POST' @{message='Build Piano Auto Sustain 1.0 VST3 and AU MIDI FX packages';tree=$newTree.sha;parents=@($parent)}
 if($exists){$null=Api "git/refs/heads/$branch" 'PATCH' @{sha=$newCommit.sha;force=$false}}
 else {$null=Api 'git/refs' 'POST' @{ref="refs/heads/$branch";sha=$newCommit.sha}}
 Write-Output "Uploaded $($files.Count) source files to $branch at $($newCommit.sha)"
}
}
