# Builds the deSIDuous demo video from a RenderTest --demo recording.
#
#   1. Record:   RenderTest.exe <deSIDuous.vst3> <workdir> --demo tools\demo\demo-script.txt <path to ffmpeg.exe>
#   2. Assemble: powershell -File tools\demo\make-video.ps1 -WorkDir <workdir>
#
# Layout (1920x1080): the plugin window on the left, a caption panel on the right
# (logo, preset name and description from the script's "caption" lines), a title card
# at the start and an end card with the download link.

param(
    [string]$WorkDir = "$PSScriptRoot\..\..\renders\demo",
    [string]$Script  = "$PSScriptRoot\demo-script.txt",
    [string]$Logo    = "$PSScriptRoot\..\..\Resources\logo.png",
    [string]$Out     = "deSIDuous-demo.mp4",
    [double]$VisualLag = 0.10            # the editor repaints on 20-30 Hz timers, so the picture trails the sound
)
$ErrorActionPreference = 'Stop'
Set-Location $WorkDir
$ff = (Get-Command ffmpeg).Source

# ---- sync: first frame where the white cover over the plugin has gone -------------
$null = & $ff -v error -t 15 -i ui.mkv -vf "crop=400:200:540:400,signalstats,metadata=print:key=lavfi.signalstats.YAVG:file=sync.txt" -f null - 2>&1
$frame = $null
foreach ($line in Get-Content sync.txt) {
    if ($line -match 'frame:(\d+)') { $f = [int]$Matches[1] }
    elseif ($line -match 'YAVG=([\d.]+)' -and [double]$Matches[1] -lt 200) { $frame = $f; break }
}
if ($null -eq $frame) { throw "sync marker not found in ui.mkv" }
$offset = ($frame - 0.5) / 30.0 + $VisualLag   # cover vanished between frames $frame-1 and $frame
"music starts at {0:N3} s into the capture (frame $frame)" -f $offset

# ---- captions and timing from the demo script ---------------------------------------
$captions = @()
$end = 0.0
foreach ($line in Get-Content $Script) {
    if ($line -match '^caption\s+([\d.]+)\s+([\d.]+)\s+(.+)$') {
        $parts = $Matches[3].Split('|') | ForEach-Object { $_.Trim() }
        $captions += [pscustomobject]@{ T0 = [double]$Matches[1]; T1 = [double]$Matches[2]; Lines = $parts }
    }
    elseif ($line -match '^end\s+([\d.]+)') { $end = [double]$Matches[1] }
}
$tail = 2.5                              # end card holds after the music
$total = $end + $tail
$endCardAt = $end - 3.0

# Text goes through files so commas, colons and apostrophes need no escaping
$utf8 = New-Object System.Text.UTF8Encoding($false)
function TextFile([string]$name, [string]$text) { [IO.File]::WriteAllText((Join-Path $WorkDir $name), $text, $utf8); $name }
Copy-Item "$env:WINDIR\Fonts\segoeuib.ttf" bold.ttf -Force
Copy-Item "$env:WINDIR\Fonts\segoeui.ttf" regular.ttf -Force
Copy-Item "$env:WINDIR\Fonts\consola.ttf" mono.ttf -Force
Copy-Item $Logo logo.png -Force

$cream = '0xfff2dc'; $lilac = '0xc4bcf5'; $cyan = '0x19eef4'; $blue = '0x1a9be6'; $night = '0x020713'
function Draw([string]$font, [int]$size, [string]$color, [string]$file, [string]$x, [string]$y, [string]$extra = '') {
    "drawtext=fontfile=${font}:textfile=${file}:fontsize=${size}:fontcolor=${color}:x=${x}:y=${y}$extra"
}

# ---- static caption panel --------------------------------------------------------------
$panel = @(
    (Draw 'mono.ttf' 19 $cyan (TextFile 't_chip.txt' 'MOS 6581 / 8580 SID SYNTH') 1532 168),
    "drawbox=x=1532:y=206:w=356:h=3:color=${cyan}:t=fill",
    (Draw 'mono.ttf' 20 $blue (TextFile 't_now.txt' 'NOW PLAYING') 1532 262),
    (Draw 'bold.ttf' 25 $cream (TextFile 't_free.txt' 'Free and open source') 1532 880),
    (Draw 'regular.ttf' 22 $lilac (TextFile 't_platforms.txt' 'VST3 for Windows, macOS and Linux') 1532 920),
    (Draw 'mono.ttf' 19 $cyan (TextFile 't_url.txt' 'github.com/squibsy/deSIDuous') 1532 1000)
)

# ---- per-segment captions ----------------------------------------------------------------
$i = 0
$segment = foreach ($c in $captions) {
    $en = ":enable='between(t,$($c.T0),$($c.T1))'"
    Draw 'bold.ttf' 31 $cream (TextFile "c${i}_0.txt" $c.Lines[0]) 1532 298 $en
    for ($l = 1; $l -lt $c.Lines.Count; $l++) {
        Draw 'regular.ttf' 23 $lilac (TextFile "c${i}_$l.txt" $c.Lines[$l]) 1532 (322 + 40 * $l) $en
    }
    $i++
}

# ---- title and end cards -----------------------------------------------------------------
$intro = @(
    "color=c=${night}:s=1920x1080:r=30:d=4,format=rgba[ibg];",
    "[lg_intro]scale=1300:-1[lgi];",
    "[ibg][lgi]overlay=(W-w)/2:330,",
    ((Draw 'bold.ttf' 46 $cream (TextFile 'i_1.txt' 'MOS 6581 / 8580 SID chip synthesizer') '(w-text_w)/2' 600) + ','),
    ((Draw 'regular.ttf' 32 $lilac (TextFile 'i_2.txt' 'VST3 instrument for Windows, macOS and Linux') '(w-text_w)/2' 675) + ','),
    "fade=t=out:st=3.0:d=0.7:alpha=1[intro]"
) -join ''
$outro = @(
    "color=c=${night}:s=1920x1080:r=30:d=$($total - $endCardAt),format=rgba[obg];",
    "[lg_outro]scale=1100:-1[lgo];",
    "[obg][lgo]overlay=(W-w)/2:330,",
    ((Draw 'bold.ttf' 50 $cream (TextFile 'o_1.txt' 'Free and open source') '(w-text_w)/2' 560) + ','),
    ((Draw 'regular.ttf' 34 $lilac (TextFile 'o_2.txt' 'VST3 for Windows, macOS and Linux') '(w-text_w)/2' 635) + ','),
    ((Draw 'mono.ttf' 40 $cyan (TextFile 'o_3.txt' 'github.com/squibsy/deSIDuous') '(w-text_w)/2' 725) + ','),
    "fade=t=in:st=0:d=0.7:alpha=1,setpts=PTS+$endCardAt/TB[outro]"
) -join ''

$graph = @(
    "color=c=${night}:s=1920x1080:r=30:d=$total[bg]",
    "[0:v]setpts=PTS-STARTPTS,format=rgba[ui]",
    "[2:v]split=3[lg_panel][lg_intro][lg_outro]",
    "[lg_panel]scale=356:-1[lgp]",
    $intro,
    $outro,
    ("[bg][ui]overlay=20:40:eof_action=repeat[v1];[v1][lgp]overlay=1532:72[v2];[v2]" + (($panel + $segment) -join ',') + "[v3]"),
    "[v3][intro]overlay=0:0:eof_action=pass[v4]",
    "[v4][outro]overlay=0:0:eof_action=pass,format=yuv420p[vout]",
    "[1:a]volume=-2dB,alimiter=limit=0.85:level=false,apad,atrim=0:$total,afade=t=out:st=$($total - 1.2):d=1.2[aout]"
) -join ";`n"
[IO.File]::WriteAllText((Join-Path $WorkDir 'graph.txt'), $graph, $utf8)

$ErrorActionPreference = 'Continue'   # ffmpeg reports progress on stderr
& $ff -y -v error -ss $offset -i ui.mkv -i audio.wav -loop 1 -i logo.png `
    -filter_complex_script graph.txt -map "[vout]" -map "[aout]" -t $total `
    -c:v libx264 -preset slow -crf 18 -r 30 -c:a aac -b:a 192k -movflags +faststart $Out
if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed" }
Get-Item $Out | Select-Object Name, Length
