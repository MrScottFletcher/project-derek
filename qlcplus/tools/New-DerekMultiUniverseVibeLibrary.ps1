param(
    [string]$SourceWorkspace = (Join-Path $PSScriptRoot '..\dereks-animation-library-v02.qxw'),
    [string]$OutputWorkspace = (Join-Path $PSScriptRoot '..\dereks-animation-library-v03.qxw')
)

$ErrorActionPreference = 'Stop'

function Add-Element([System.Xml.XmlNode]$Parent, [string]$Name, [hashtable]$Attributes = @{}, [string]$Text = $null) {
    $node = $Parent.OwnerDocument.CreateElement($Name, $Parent.NamespaceURI)
    foreach ($key in $Attributes.Keys) { [void]$node.SetAttribute($key, [string]$Attributes[$key]) }
    if ($null -ne $Text) { $node.InnerText = $Text }
    [void]$Parent.AppendChild($node)
    return $node
}

function Convert-U1FixtureValue([string]$FixtureId, [string]$ValueText, [hashtable]$FixtureMap, [int[]]$U1Centers, [int]$DestinationCenter) {
    $newFixtureId = [string]$FixtureMap[$FixtureId]
    $parts = $ValueText.Split(',')
    if ($FixtureId -match '^[0-7]$') {
        # Convert U1's per-Derek centers only. Raw 192/64 remain the physically
        # observed U1 left/right convention and are copied as the U2/U3 test default.
        for ($i = 0; $i -lt $parts.Count; $i += 2) {
            if ([int]$parts[$i] -eq 0 -and $U1Centers -contains [int]$parts[$i + 1]) {
                $parts[$i + 1] = [string]$DestinationCenter
            }
        }
    }
    return [pscustomobject]@{ Id=$newFixtureId; Text=($parts -join ',') }
}

function Clone-Scene([System.Xml.XmlNode]$Engine, [System.Xml.XmlNode]$SourceScene, [int]$NewId, [string]$UniverseLabel, [hashtable]$FixtureMap, [int[]]$U1Centers, [int]$DestinationCenter) {
    $newName = $SourceScene.Name -replace 'LIB U1', "LIB $UniverseLabel"
    $scene = Add-Element $Engine 'Function' @{ ID=$NewId; Type='Scene'; Name=$newName }
    [void](Add-Element $scene 'Speed' @{ FadeIn=$SourceScene.Speed.FadeIn; FadeOut=$SourceScene.Speed.FadeOut; Duration=$SourceScene.Speed.Duration })
    foreach ($value in @($SourceScene.FixtureVal)) {
        $converted = Convert-U1FixtureValue $value.ID ([string]$value.'#text') $FixtureMap $U1Centers $DestinationCenter
        [void](Add-Element $scene 'FixtureVal' @{ ID=$converted.Id } $converted.Text)
    }
}

function Clone-Chaser([System.Xml.XmlNode]$Engine, [System.Xml.XmlNode]$SourceChaser, [int]$NewId, [string]$UniverseLabel, [hashtable]$FunctionMap) {
    $newName = $SourceChaser.Name -replace 'LIB U1', "LIB $UniverseLabel"
    $chaser = Add-Element $Engine 'Function' @{ ID=$NewId; Type='Chaser'; Name=$newName }
    [void](Add-Element $chaser 'Speed' @{ FadeIn=$SourceChaser.Speed.FadeIn; FadeOut=$SourceChaser.Speed.FadeOut; Duration=$SourceChaser.Speed.Duration })
    [void](Add-Element $chaser 'Direction' @{} ([string]$SourceChaser.Direction))
    [void](Add-Element $chaser 'RunOrder' @{} ([string]$SourceChaser.RunOrder))
    [void](Add-Element $chaser 'SpeedModes' @{ FadeIn=$SourceChaser.SpeedModes.FadeIn; FadeOut=$SourceChaser.SpeedModes.FadeOut; Duration=$SourceChaser.SpeedModes.Duration })
    foreach ($step in @($SourceChaser.Step)) {
        $newTarget = $FunctionMap[[string]$step.'#text']
        [void](Add-Element $chaser 'Step' @{ Number=$step.Number; FadeIn=$step.FadeIn; Hold=$step.Hold; FadeOut=$step.FadeOut } $newTarget)
    }
}

function Add-ControlPage([System.Xml.XmlNode]$VirtualConsole, [int]$FrameId, [string]$UniverseLabel, [int]$ParkFunctionId, [object[]]$VibeFunctions) {
    $frame = Add-Element $VirtualConsole 'Frame' @{ Caption="Vibe Library - $UniverseLabel"; ID=$FrameId }
    $appearance = Add-Element $frame 'Appearance' @{}
    [void](Add-Element $appearance 'Font' @{} 'Roboto Condensed,16,-1,5,400,0,0,0,0,0,0,0,0,0,0,1')
    [void](Add-Element $frame 'WindowState' @{ Visible='True'; X=0; Y=0; Width=1920; Height=1080 })
    [void](Add-Element $frame 'AllowResize' @{} 'False'); [void](Add-Element $frame 'ShowHeader' @{} 'True'); [void](Add-Element $frame 'ShowEnableButton' @{} 'True'); [void](Add-Element $frame 'Collapsed' @{} 'False'); [void](Add-Element $frame 'Disabled' @{} 'False')
    $items = @([pscustomobject]@{Caption='PARK / BLACKOUT'; Id=$ParkFunctionId}) + $VibeFunctions
    for ($i = 0; $i -lt $items.Count; $i++) {
        $column = $i % 6; $row = [math]::Floor($i / 6)
        $button = Add-Element $frame 'Button' @{ Caption=$items[$i].Caption; ID=$i; Icon='' }
        [void](Add-Element $button 'WindowState' @{ Visible='True'; X=(20 + $column * 310); Y=(40 + $row * 110); Width=290; Height=88 })
        $buttonAppearance = Add-Element $button 'Appearance' @{}
        [void](Add-Element $buttonAppearance 'BackgroundColor' @{} '4282664004')
        [void](Add-Element $button 'Function' @{ ID=$items[$i].Id })
        [void](Add-Element $button 'Action' @{} 'Toggle')
    }
}

if (-not (Test-Path -LiteralPath $SourceWorkspace)) { throw "Source workspace was not found: $SourceWorkspace" }
[xml]$workspace = Get-Content -Raw -LiteralPath $SourceWorkspace
$engine = $workspace.Workspace.Engine
$functions = @($engine.Function)
$functionById = @{}
foreach ($function in $functions) { $functionById[[string]$function.ID] = $function }
if (($functions | Measure-Object).Count -ne 398) { throw 'Expected the unmodified v02 library with 398 functions.' }

$u1CueScenes = @($functions | Where-Object {$_.Type -eq 'Scene' -and $_.Name -like 'LIB U1 Vibe Cue - *'})
$u1VibeChasers = @($functions | Where-Object {$_.Type -eq 'Chaser' -and $_.Name -like 'LIB U1 Vibe - *'})
if ($u1CueScenes.Count -ne 306 -or $u1VibeChasers.Count -ne 51) { throw 'Expected 306 U1 vibe cue scenes and 51 U1 vibe chasers.' }

$u1Centers = @(129,137,140,139,133,133,134,133)
$u1Buttons = @()
foreach ($chaser in $u1VibeChasers) {
    $u1Buttons += [pscustomobject]@{Caption=($chaser.Name -replace 'LIB U1 Vibe - ', ''); Id=[int]$chaser.ID}
}
Add-ControlPage $workspace.Workspace.VirtualConsole 2 'U1' 15 $u1Buttons
$universes = @(
    @{Label='U2'; Controller='17'; Dereks=@('9','10','11','12','13','14','15','16'); FrameId=3},
    @{Label='U3'; Controller='19'; Dereks=@('20','21','22','23','24','25','26','27'); FrameId=4}
)
$nextId = 398
$allControlPages = @()
foreach ($universe in $universes) {
    $fixtureMap = @{ '8'=$universe.Controller }
    for ($i = 0; $i -lt 8; $i++) { $fixtureMap[[string]$i] = $universe.Dereks[$i] }
    $functionMap = @{}
    # Clone the U1 park scene first, then all low-level cue scenes, then the chasers that use them.
    $parkId = $nextId; $nextId++
    Clone-Scene $engine $functionById['15'] $parkId $universe.Label $fixtureMap $u1Centers 128
    foreach ($scene in $u1CueScenes) {
        $newId = $nextId; $nextId++
        $functionMap[[string]$scene.ID] = $newId
        Clone-Scene $engine $scene $newId $universe.Label $fixtureMap $u1Centers 128
    }
    $buttons = @()
    foreach ($chaser in $u1VibeChasers) {
        $newId = $nextId; $nextId++
        Clone-Chaser $engine $chaser $newId $universe.Label $functionMap
        $caption = ($chaser.Name -replace 'LIB U1 Vibe - ', '')
        $buttons += [pscustomobject]@{Caption=$caption; Id=$newId}
    }
    Add-ControlPage $workspace.Workspace.VirtualConsole $universe.FrameId $universe.Label $parkId $buttons
    $allControlPages += [pscustomobject]@{Universe=$universe.Label; ParkId=$parkId; FirstVibeId=$buttons[0].Id; LastVibeId=$buttons[$buttons.Count - 1].Id}
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$writer = [System.Xml.XmlWriter]::Create($OutputWorkspace, $settings)
$workspace.Save($writer); $writer.Dispose()

$catalogPath = Join-Path (Split-Path -Parent $OutputWorkspace) 'Derek-Animation-Catalog-v03.md'
$mappingPath = Join-Path (Split-Path -Parent $OutputWorkspace) 'Derek-QLC-Mapping-Reference-v03.md'
@'
# Derek animation catalog v03 — independent U1, U2, and U3 vibe libraries

v03 contains the same 51 one-shot vibe chasers for each universe: `LIB U1 Vibe`, `LIB U2 Vibe`, and `LIB U3 Vibe`. Each instance is independent and controls only its own cluster controller plus its eight Derek fixtures.

## Operator controls

- The existing `Animation Library - U1 Pilot` page remains available for U1 pilot functions.
- `Vibe Library - U1`, `Vibe Library - U2`, and `Vibe Library - U3` Virtual Console pages each provide Park/Blackout plus all 51 vibe buttons.
- Do not run two functions for the same universe at the same time. Different universe functions can run independently.

## Content and timing

Each universe has Angry, Aggressive, Nervous, Happy, Fun, Celebration, Reverent, Sad, Welcoming, Chaotic, Cheering, Zombie, Super Hero, Patriotic, Playful, Rainbow Fun, and Sparkle Ponies; each has three variations. Cue steps are at most 3.6 seconds and variations run approximately 10, 15, or 22 seconds. Every chaser ends in that universe's Park/Blackout pose.

## Required commissioning

U2 and U3 use raw DMX center 128 and the U1 physical direction convention (192 = left; 64 = right) as initial generated values. These are **not physically verified U2/U3 calibrations**. Before live playback, run each universe's Park/Blackout and a short vibe with clear mechanical travel, then record and apply any per-Derek center/direction corrections.
'@ | Set-Content -LiteralPath $catalogPath -Encoding utf8

@'
# Derek QLC+ mapping reference v03

## Independent universe ownership

| Library prefix | Controller fixture | Derek fixture IDs | Virtual Console page |
| --- | ---: | --- | --- |
| LIB U1 | 8 | 0-7 | Vibe Library - U1 |
| LIB U2 | 17 | 9-16 | Vibe Library - U2 |
| LIB U3 | 19 | 20-27 | Vibe Library - U3 |

Each function writes only its listed universe's controller and Derek fixtures. U1 uses its tested centers. U2/U3 use generated raw center 128 and must be commissioned physically. All libraries use conservative lift values and firmware-enforced can-profile limits.

The separate `LIB Ux Park / Blackout` scene is the explicit safe stop/reset for each universe. Do not depend on a QLC+ blackout or stopping a chaser to park mechanical motion.
'@ | Set-Content -LiteralPath $mappingPath -Encoding utf8

Write-Host "Created $OutputWorkspace"
Write-Host "Created $catalogPath"
Write-Host "Created $mappingPath"
$allControlPages | Format-Table -AutoSize
