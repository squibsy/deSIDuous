# Captures the manual's screenshots from the real plugin.
# Opens the editor via RenderTest --ui <program> (which holds a chord), grabs the
# window's client area, and saves the full UI and/or cropped panels.
#
#   powershell -File tools\screenshots.ps1
#
# Panel rectangles are in editor (logical) pixels and match DesiduousEditor::resized().

param(
    [string]$Plugin = "$PSScriptRoot\..\build\deSIDuous_artefacts\Release\VST3\deSIDuous.vst3",
    [string]$Host_  = "$PSScriptRoot\..\build\RenderTest_artefacts\Release\RenderTest.exe",
    [string]$OutDir = "$PSScriptRoot\..\docs\images",
    [double]$Scale  = 1.25,     # output pixels per editor pixel
    [string[]]$Only = @()       # optional: only these program numbers
)

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class Win {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
'@
[Win]::SetProcessDPIAware() | Out-Null

$editorW = 1184
$panels = @{
    full     = @(0, 0, 1184, 800)
    header   = @(0, 0, 1184, 84)
    voice1   = @(8, 92, 384, 250)
    voice2   = @(400, 92, 384, 250)
    voice3   = @(792, 92, 384, 250)
    voices   = @(8, 92, 1168, 250)
    filter   = @(8, 350, 384, 230)
    mod      = @(400, 350, 384, 230)
    global   = @(792, 350, 384, 230)
    scope    = @(8, 588, 776, 130)
    regs     = @(792, 588, 384, 130)
    bottom   = @(8, 588, 1168, 130)
    keyboard = @(0, 726, 1184, 74)
}

# program index -> list of "panel:filename"
$shots = [ordered]@{
    '21' = @('full:ui-overview', 'header:header', 'global:chip-output', 'bottom:scope-registers', 'voices:three-voices')
    '2' = @('voice1:voice-sync')
    '36' = @('voice1:voice-ring')
    '43' = @('voice1:voice-drum-hit')
    '4' = @('filter:filter-acid')
    '27' = @('mod:mod-arp')
    '10' = @('mod:mod-vibrato')
    '0' = @('scope:scope-saw')
    '5' = @('scope:scope-triangle')
    '14' = @('scope:scope-pulse')
    '59' = @('scope:scope-tri-pulse')
    '57' = @('scope:scope-noise')
}

New-Item -ItemType Directory -Force $OutDir | Out-Null

foreach ($program in $shots.Keys) {
    if ($Only.Count -gt 0 -and $Only -notcontains $program) { continue }
    $p = Start-Process -FilePath $Host_ -ArgumentList "`"$Plugin`"", "`"$env:TEMP`"", '--ui', $program -PassThru
    Start-Sleep -Milliseconds 4500
    $p.Refresh()
    $h = $p.MainWindowHandle
    [Win]::SetForegroundWindow($h) | Out-Null
    Start-Sleep -Milliseconds 300

    $rc = New-Object Win+RECT; [Win]::GetClientRect($h, [ref]$rc) | Out-Null
    $pt = New-Object Win+POINT; [Win]::ClientToScreen($h, [ref]$pt) | Out-Null
    $cw = $rc.R - $rc.L; $ch = $rc.B - $rc.T
    $grab = New-Object System.Drawing.Bitmap $cw, $ch
    $g = [System.Drawing.Graphics]::FromImage($grab)
    $g.CopyFromScreen($pt.X, $pt.Y, 0, 0, $grab.Size)
    $g.Dispose()
    $k = $cw / $editorW   # screen pixels per editor pixel

    # Safety check: the header background must be the logo's night-black (2,7,19).
    # If anything else is on screen there, save nothing for this program.
    $probe = $grab.GetPixel([int](700 * $k), [int](8 * $k))
    if ($cw -lt 1000 -or [Math]::Abs($probe.R - 2) -gt 6 -or [Math]::Abs($probe.G - 7) -gt 6 -or [Math]::Abs($probe.B - 19) -gt 6) {
        Write-Warning "program ${program}: capture does not show the plugin (probe $($probe.R),$($probe.G),$($probe.B)), skipped"
        $grab.Dispose()
        if (-not $p.HasExited) { $p.Kill() }
        continue
    }

    foreach ($spec in $shots[$program]) {
        $panel, $name = $spec.Split(':')
        $r = $panels[$panel]
        $src = New-Object System.Drawing.Rectangle ([int]($r[0] * $k)), ([int]($r[1] * $k)), ([int]($r[2] * $k)), ([int]($r[3] * $k))
        $src.Width = [Math]::Min($src.Width, $cw - $src.X); $src.Height = [Math]::Min($src.Height, $ch - $src.Y)
        $dw = [int]($r[2] * $Scale); $dh = [int]($r[3] * $Scale)
        $out = New-Object System.Drawing.Bitmap $dw, $dh
        $og = [System.Drawing.Graphics]::FromImage($out)
        $og.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $og.DrawImage($grab, (New-Object System.Drawing.Rectangle 0, 0, $dw, $dh), $src, [System.Drawing.GraphicsUnit]::Pixel)
        $og.Dispose()
        $file = Join-Path $OutDir "$name.png"
        $out.Save($file, [System.Drawing.Imaging.ImageFormat]::Png)
        $out.Dispose()
        "program $program -> $file"
    }
    $grab.Dispose()
    if (-not $p.HasExited) { $p.Kill() }
    Start-Sleep -Milliseconds 300
}
