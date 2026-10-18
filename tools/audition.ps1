# Joins RenderTest's per-preset WAVs into audition files: one MP3 per category plus one
# with every preset, each with a track list giving the start time of every sound.
#
#   powershell -File tools\audition.ps1 [-Renders renders] [-Out renders\audition]

param(
    [string]$Renders = "$PSScriptRoot\..\renders",
    [string]$Out     = "$PSScriptRoot\..\renders\audition",
    [double]$Length  = 3.0,    # seconds kept from each render
    [double]$Gap     = 0.35
)
$ErrorActionPreference = 'Stop'
$ff = (Get-Command ffmpeg).Source
New-Item -ItemType Directory -Force $Out, "$Out\clips" | Out-Null

# Renders are named NNN_Category_Preset name.wav
$items = Get-ChildItem $Renders -Filter '*.wav' | Where-Object { $_.Name -match '^(\d{3})_([^_]+)_(.+)\.wav$' } |
    ForEach-Object { [pscustomobject]@{ File = $_.FullName; Index = [int]$Matches[1]; Category = $Matches[2]; Name = $Matches[3] } } |
    Sort-Object Index

$ErrorActionPreference = 'Continue'   # ffmpeg writes to stderr
foreach ($it in $items) {
    $clip = Join-Path "$Out\clips" ('{0:D3}.wav' -f $it.Index)
    & $ff -v error -y -i $it.File -af "atrim=0:$Length,afade=t=out:st=$($Length - 0.3):d=0.3,apad=pad_dur=$Gap" -ar 48000 -ac 2 -c:a pcm_s16le $clip
    $it | Add-Member -NotePropertyName Clip -NotePropertyValue $clip
}

function Join-Clips([object[]]$set, [string]$title) {
    $safe = $title -replace '[^\w\- ]', ''
    $list = Join-Path "$Out\clips" "$safe.txt"
    $set | ForEach-Object { "file '$($_.Clip.Replace('\', '/'))'" } | Set-Content -Encoding ascii $list
    & $ff -v error -y -f concat -safe 0 -i $list -c:a libmp3lame -b:a 192k (Join-Path $Out "$safe.mp3")

    $t = 0.0
    $lines = foreach ($s in $set) { '{0:mm\:ss}  {1:D3}  {2}' -f [TimeSpan]::FromSeconds($t), ($s.Index + 1), $s.Name; $t += $Length + $Gap }
    $lines | Set-Content -Encoding utf8 (Join-Path $Out "$safe.txt")
}

foreach ($group in ($items | Group-Object Category)) {
    Join-Clips $group.Group $group.Name
}
Join-Clips $items 'All presets'
Remove-Item -Recurse -Force "$Out\clips"
Get-ChildItem $Out -Filter '*.mp3' | Select-Object Name, @{ n = 'MB'; e = { [Math]::Round($_.Length / 1MB, 1) } }
