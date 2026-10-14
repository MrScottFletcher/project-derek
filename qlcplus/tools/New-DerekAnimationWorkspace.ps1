param(
    [string]$SourceWorkspace = (Join-Path $PSScriptRoot '..\dereks-workspace-v01.qxw'),
    [string]$OutputWorkspace = (Join-Path $PSScriptRoot '..\dereks-animation-library-v01.qxw')
)

$ErrorActionPreference = 'Stop'

function Add-Element([System.Xml.XmlNode]$Parent, [string]$Name, [hashtable]$Attributes = @{}, [string]$Text = $null) {
    $node = $Parent.OwnerDocument.CreateElement($Name, $Parent.NamespaceURI)
    foreach ($key in $Attributes.Keys) { [void]$node.SetAttribute($key, [string]$Attributes[$key]) }
    if ($null -ne $Text) { $node.InnerText = $Text }
    [void]$Parent.AppendChild($node)
    return $node
}

function Add-Scene([System.Xml.XmlNode]$Engine, [int]$Id, [string]$Name, [hashtable]$FixtureValues) {
    $scene = Add-Element $Engine 'Function' @{ ID = $Id; Type = 'Scene'; Name = $Name }
    [void](Add-Element $scene 'Speed' @{ FadeIn = 0; FadeOut = 0; Duration = 0 })
    foreach ($fixtureId in ($FixtureValues.Keys | Sort-Object {[int]$_})) {
        [void](Add-Element $scene 'FixtureVal' @{ ID = $fixtureId } $FixtureValues[$fixtureId])
    }
}

function Add-Chaser([System.Xml.XmlNode]$Engine, [int]$Id, [string]$Name, [string]$RunOrder, [object[]]$Steps) {
    $duration = 0
    foreach ($step in $Steps) { $duration += [int]$step.FadeIn + [int]$step.Hold + [int]$step.FadeOut }
    $chaser = Add-Element $Engine 'Function' @{ ID = $Id; Type = 'Chaser'; Name = $Name }
    [void](Add-Element $chaser 'Speed' @{ FadeIn = 0; FadeOut = 0; Duration = $duration })
    [void](Add-Element $chaser 'Direction' @{} 'Forward')
    [void](Add-Element $chaser 'RunOrder' @{} $RunOrder)
    [void](Add-Element $chaser 'SpeedModes' @{ FadeIn = 'Default'; FadeOut = 'Default'; Duration = 'Common' })
    for ($i = 0; $i -lt $Steps.Count; $i++) {
        $step = $Steps[$i]
        [void](Add-Element $chaser 'Step' @{ Number = $i; FadeIn = $step.FadeIn; Hold = $step.Hold; FadeOut = $step.FadeOut } $step.FunctionId)
    }
}

function Derek-Values([int]$Pan, [int]$Lift, [int[]]$Spotlight = @(0,0,0), [int[]]$RightEye = @(0,0,0), [int[]]$LeftEye = @(0,0,0)) {
    return "0,$Pan,1,$Lift,2,$($Spotlight[0]),3,$($Spotlight[1]),4,$($Spotlight[2]),5,$($RightEye[0]),6,$($RightEye[1]),7,$($RightEye[2]),8,$($LeftEye[0]),9,$($LeftEye[1]),10,$($LeftEye[2])"
}

function Controller-Values([int]$Speed) {
    # Active=8, Apply Outputs=255, Emergency/Status/Discovery/Audio=0, then speed.
    return "0,8,1,255,2,0,3,0,4,0,5,0,6,0,7,0,8,0,9,0,10,0,11,$Speed,12,0,13,0"
}

function U1-SceneValues([int]$Pan, [int]$Lift, [int]$Speed, [int[]]$Spotlight = @(0,0,0), [int[]]$RightEye = @(0,0,0), [int[]]$LeftEye = @(0,0,0)) {
    $values = @{ '8' = (Controller-Values $Speed) }
    $centers = @(129, 137, 140, 139, 133, 133, 134, 133)
    for ($i = 0; $i -lt 8; $i++) {
        $resolvedPan = if ($Pan -lt 0) { $centers[$i] } else { $Pan }
        $values[[string]$i] = Derek-Values $resolvedPan $Lift $Spotlight $RightEye $LeftEye
    }
    return $values
}

function U1-WaveValues([int]$RaisedCount, [int]$Speed) {
    $values = @{ '8' = (Controller-Values $Speed) }
    $centers = @(129, 137, 140, 139, 133, 133, 134, 133)
    for ($i = 0; $i -lt 8; $i++) {
        $lift = if ($i -lt $RaisedCount) { 96 } else { 0 }
        $light = if ($i -lt $RaisedCount) { @(24, 8, 0) } else { @(0, 0, 0) }
        $values[[string]$i] = Derek-Values $centers[$i] $lift $light $light $light
    }
    return $values
}

if (-not (Test-Path -LiteralPath $SourceWorkspace)) { throw "Source workspace was not found: $SourceWorkspace" }
[xml]$workspace = Get-Content -Raw -LiteralPath $SourceWorkspace
$engine = $workspace.Workspace.Engine
$existingFunctions = @($engine.Function)
$maxId = [int](($existingFunctions | ForEach-Object { [int]$_.ID } | Measure-Object -Maximum).Maximum)
if ($maxId -ge 15) { throw 'This generator expects the original workspace only; output functions already appear to be present.' }

# DMX values are deliberately conservative. Although MCU source calls the lower
# target "left", physical U1 testing proves this installation is mechanically
# inverted: DMX 192 is physical left and DMX 64 is physical right. Lift 75/96/128
# maps to approximately 50/64/85 degrees before each cluster applies its own
# persisted can-profile clamp.
$ids = @{
    Park = 15; Peek = 16; RaisedCenter = 17; RaisedLeft = 18; RaisedRight = 19;
    WarmLow = 20; WarmHigh = 21; RedLow = 22; BlueLow = 23;
    WavePark = 24; Wave1 = 25; Wave2 = 26; Wave3 = 27; Wave4 = 28; Wave5 = 29; Wave6 = 30; Wave7 = 31; Wave8 = 32;
    PeekHide = 33; RiseLookReturn = 34; DoubleTake = 35; PopUpWave = 36; AllEyes = 37; QuietIdle = 38; StartleRetreat = 39; LedPulse = 40
}

Add-Scene $engine $ids.Park 'LIB U1 Park / Blackout' (U1-SceneValues -1 0 170)
Add-Scene $engine $ids.Peek 'LIB U1 Peek (center)' (U1-SceneValues -1 75 170 @(12,4,0) @(20,8,0) @(20,8,0))
Add-Scene $engine $ids.RaisedCenter 'LIB U1 Raised / Center' (U1-SceneValues -1 128 100 @(12,4,0) @(32,12,0) @(32,12,0))
Add-Scene $engine $ids.RaisedLeft 'LIB U1 Raised / Look Left' (U1-SceneValues 192 128 100 @(12,4,0) @(32,12,0) @(32,12,0))
Add-Scene $engine $ids.RaisedRight 'LIB U1 Raised / Look Right' (U1-SceneValues 64 128 100 @(12,4,0) @(32,12,0) @(32,12,0))
Add-Scene $engine $ids.WarmLow 'LIB U1 LEDs Warm Low' (U1-SceneValues -1 0 170 @(12,3,0) @(24,6,0) @(24,6,0))
Add-Scene $engine $ids.WarmHigh 'LIB U1 LEDs Warm High' (U1-SceneValues -1 0 170 @(42,10,0) @(80,20,0) @(80,20,0))
Add-Scene $engine $ids.RedLow 'LIB U1 LEDs Red Low' (U1-SceneValues -1 0 170 @(20,0,0) @(48,0,0) @(48,0,0))
Add-Scene $engine $ids.BlueLow 'LIB U1 LEDs Blue Low' (U1-SceneValues -1 0 170 @(0,4,20) @(0,10,48) @(0,10,48))
Add-Scene $engine $ids.WavePark 'LIB U1 Wave / Park' (U1-WaveValues 0 170)
for ($i = 1; $i -le 8; $i++) { Add-Scene $engine $ids["Wave$i"] "LIB U1 Wave / $i Raised" (U1-WaveValues $i 100) }

Add-Chaser $engine $ids.PeekHide 'LIB U1 Peek and Hide (3.4 s)' 'SingleShot' @(
    [pscustomobject]@{ FunctionId=$ids.Peek; FadeIn=1200; Hold=900; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.Park; FadeIn=1300; Hold=0; FadeOut=0 }
)
Add-Chaser $engine $ids.RiseLookReturn 'LIB U1 Rise, Look, Return (10.0 s)' 'SingleShot' @(
    [pscustomobject]@{ FunctionId=$ids.RaisedCenter; FadeIn=1800; Hold=900; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedLeft; FadeIn=650; Hold=1100; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedCenter; FadeIn=650; Hold=800; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedRight; FadeIn=650; Hold=1100; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.Park; FadeIn=1800; Hold=0; FadeOut=0 }
)
Add-Chaser $engine $ids.DoubleTake 'LIB U1 Double Take (6.1 s)' 'SingleShot' @(
    [pscustomobject]@{ FunctionId=$ids.RaisedLeft; FadeIn=900; Hold=700; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedRight; FadeIn=500; Hold=800; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedLeft; FadeIn=500; Hold=700; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.Park; FadeIn=2000; Hold=0; FadeOut=0 }
)
$waveSteps = @()
for ($i = 1; $i -le 8; $i++) { $waveSteps += [pscustomobject]@{ FunctionId=$ids["Wave$i"]; FadeIn=300; Hold=150; FadeOut=0 } }
$waveSteps += [pscustomobject]@{ FunctionId=$ids.WavePark; FadeIn=1600; Hold=0; FadeOut=0 }
Add-Chaser $engine $ids.PopUpWave 'LIB U1 Pop-up Wave (5.2 s)' 'SingleShot' $waveSteps
Add-Chaser $engine $ids.AllEyes 'LIB U1 All Eyes on You (9.2 s)' 'SingleShot' @(
    [pscustomobject]@{ FunctionId=$ids.RaisedCenter; FadeIn=1600; Hold=1200; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedLeft; FadeIn=700; Hold=1000; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedRight; FadeIn=900; Hold=1200; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedCenter; FadeIn=700; Hold=700; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.Park; FadeIn=1700; Hold=0; FadeOut=0 }
)
Add-Chaser $engine $ids.QuietIdle 'LIB U1 Quiet Idle (loop)' 'Loop' @(
    [pscustomobject]@{ FunctionId=$ids.WarmLow; FadeIn=1000; Hold=2000; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.Peek; FadeIn=1400; Hold=1200; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedLeft; FadeIn=900; Hold=1300; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedRight; FadeIn=1100; Hold=1300; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.Park; FadeIn=1600; Hold=1200; FadeOut=0 }
)
Add-Chaser $engine $ids.StartleRetreat 'LIB U1 Startle and Retreat (4.2 s)' 'SingleShot' @(
    [pscustomobject]@{ FunctionId=$ids.RaisedCenter; FadeIn=700; Hold=900; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.RaisedRight; FadeIn=450; Hold=500; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.Park; FadeIn=1650; Hold=0; FadeOut=0 }
)
Add-Chaser $engine $ids.LedPulse 'LIB U1 LED Pulse (3.6 s)' 'SingleShot' @(
    [pscustomobject]@{ FunctionId=$ids.WarmLow; FadeIn=600; Hold=300; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.WarmHigh; FadeIn=500; Hold=500; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.WarmLow; FadeIn=600; Hold=300; FadeOut=0 },
    [pscustomobject]@{ FunctionId=$ids.Park; FadeIn=800; Hold=0; FadeOut=0 }
)

# Simple manual controls on a separate Virtual Console page. Button XML follows
# the QLC+ workspace schema used by the existing VC frame.
$vc = $workspace.Workspace.VirtualConsole
$frame = Add-Element $vc 'Frame' @{ Caption='Animation Library - U1 Pilot'; ID='1' }
$frameAppearance = Add-Element $frame 'Appearance' @{}
[void](Add-Element $frameAppearance 'Font' @{} 'Roboto Condensed,16,-1,5,400,0,0,0,0,0,0,0,0,0,0,1')
[void](Add-Element $frame 'WindowState' @{ Visible='True'; X=0; Y=0; Width=1920; Height=1080 })
[void](Add-Element $frame 'AllowResize' @{} 'False'); [void](Add-Element $frame 'ShowHeader' @{} 'True'); [void](Add-Element $frame 'ShowEnableButton' @{} 'True'); [void](Add-Element $frame 'Collapsed' @{} 'False'); [void](Add-Element $frame 'Disabled' @{} 'False')
$buttons = @(
    @('PARK / BLACKOUT',$ids.Park), @('PEEK + HIDE',$ids.PeekHide), @('RISE LOOK RETURN',$ids.RiseLookReturn), @('DOUBLE TAKE',$ids.DoubleTake), @('POP-UP WAVE',$ids.PopUpWave), @('ALL EYES ON YOU',$ids.AllEyes), @('QUIET IDLE LOOP',$ids.QuietIdle), @('STARTLE + RETREAT',$ids.StartleRetreat), @('LED PULSE',$ids.LedPulse)
)
for ($i = 0; $i -lt $buttons.Count; $i++) {
    $x = 40 + (($i % 3) * 250); $y = 70 + ([math]::Floor($i / 3) * 150)
    $button = Add-Element $frame 'Button' @{ Caption=$buttons[$i][0]; ID=$i; Icon='' }
    [void](Add-Element $button 'WindowState' @{ Visible='True'; X=$x; Y=$y; Width=220; Height=110 })
    $appearance = Add-Element $button 'Appearance' @{}
    [void](Add-Element $appearance 'BackgroundColor' @{} '4282664004')
    [void](Add-Element $button 'Function' @{ ID=$buttons[$i][1] })
    [void](Add-Element $button 'Action' @{} 'Toggle')
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$writer = [System.Xml.XmlWriter]::Create($OutputWorkspace, $settings)
$workspace.Save($writer); $writer.Dispose()

$catalogPath = Join-Path (Split-Path -Parent $OutputWorkspace) 'Derek-Animation-Catalog-v01.md'
$mappingPath = Join-Path (Split-Path -Parent $OutputWorkspace) 'Derek-QLC-Mapping-Reference-v01.md'
@'
# Derek animation catalog v01

This first pilot library targets **Universe 1 / Cluster 0 / Dereks 1-8 only**. It is intentionally conservative and has no audio cues. All function names start with `LIB U1`.

| Function | Duration | Playback | Owns | Ending state |
| --- | ---: | --- | --- | --- |
| Park / Blackout | immediate | scene | all U1 Derek motion and LEDs; controller | hidden, centered, LEDs off |
| Peek and Hide | 3.4 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| Rise, Look, Return | 10.0 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| Double Take | 6.1 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| Pop-up Wave | 5.2 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| All Eyes on You | 9.2 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| Quiet Idle | loop | loop | all U1 Derek motion and LEDs; controller | cycles through park |
| Startle and Retreat | 4.2 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| LED Pulse | 3.6 s | one-shot | all U1 Derek motion and LEDs; controller | parked |

Use the `Animation Library - U1 Pilot` Virtual Console page for manual playback. Stop `Quiet Idle` before starting another motion behavior. Run **Park / Blackout** before and after physical testing, and before switching to any older workspace function.

## Pilot targets

- Conservative horizontal looks use raw DMX 192 for **physical left** and 64 for **physical right**. The MCU converts these to about 130 / 50 degrees; U1's mechanical orientation is opposite the MCU source labels.
- Raised scenes use raw lift DMX 75, 96, or 128. The MCU maps this to 50, 64, or 85 degrees before the cluster clamps it to the installed can profile.
- The library uses the U1-tested pan centers: 129, 137, 140, 139, 133, 133, 134, 133.
- LED levels intentionally remain low. Spotlight, right eye, and left eye are all set together in pilot motion scenes.

## Test order

1. Verify physical left/right on a single Derek if hardware is reconfigured. This U1 pilot is calibrated from physical testing: raw DMX 192 is physical left and 64 is physical right.
2. With clear travel, run `Park / Blackout`, then `Peek and Hide`.
3. Confirm lift clearance for the whole U1 cluster, then run `Pop-up Wave` and `All Eyes on You`.
4. Test LED color/order at low brightness with `LED Pulse`.
5. Do not add U2/U3 or audio instances until U1 behavior and installed can profiles are confirmed.
'@ | Set-Content -LiteralPath $catalogPath -Encoding utf8

@'
# Derek QLC+ mapping reference v01

Verified from `derek-mcu.ino`, `derek-cluster.ino`, and the live QLC+ fixture files.

## Universe and fixture layout

Art-Net universe IDs map directly to ESP-NOW cluster/DIP IDs. A full cluster occupies 102 channels: controller 1-14, then eight 11-channel Derek fixtures at 15, 26, 37, 48, 59, 70, 81, and 92.

Each Derek: Pan; Lift; Spotlight RGB; Right Eye RGB; Left Eye RGB. The physical LED chain is spotlight x2, right eye, left eye.

## Motion contract

The MCU converts pan DMX 0-255 to target angle 10-170. Its source test sequence calls 10 left and 170 right, but physical U1 testing establishes that this installation is mechanically inverted: DMX 192 (about 130 degrees) is physical left and DMX 64 (about 50 degrees) is physical right. Lift DMX 0-255 maps to target 0-170, then the cluster enforces its current can profile. Cluster default can profiles restrict lift to 0-136, but installed NVS values can differ.

Controller channel 12 controls speed: 0-84 fast, 85-169 medium, 170-255 slow. The pilot uses slow for park/peek/LED scenes and medium for raised movement.

## Safety behavior

After 1.5 seconds without a command, cluster firmware hides active Dereks, centers pan, and blacks out LEDs. Servo output disables one second after reaching rest. The library's explicit Park scene is the operator-facing reset; do not rely on blackout as a servo park command.

## Known limitations

- Actual per-cluster can profile values are not readable from the source tree; test them on hardware.
- PIR status currently reaches MCU serial logging but is not routed back into QLC+ triggers.
- Audio is intentionally excluded pending an SD-card track map and retrigger test.
- This pilot has no U2/U3 instances or cross-cluster compositions.
'@ | Set-Content -LiteralPath $mappingPath -Encoding utf8

Write-Host "Created $OutputWorkspace"
Write-Host "Created $catalogPath"
Write-Host "Created $mappingPath"
