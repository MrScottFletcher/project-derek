param(
    [string]$SourceWorkspace = (Join-Path $PSScriptRoot '..\dereks-animation-library-v03.qxw'),
    [string]$OutputWorkspace = (Join-Path $PSScriptRoot '..\dereks-animation-library-v04.qxw')
)

$ErrorActionPreference = 'Stop'

function Add-Element([System.Xml.XmlNode]$Parent, [string]$Name, [hashtable]$Attributes = @{}, [string]$Text = $null) {
    $node = $Parent.OwnerDocument.CreateElement($Name, $Parent.NamespaceURI)
    foreach ($key in $Attributes.Keys) { [void]$node.SetAttribute($key, [string]$Attributes[$key]) }
    if ($null -ne $Text) { $node.InnerText = $Text }
    [void]$Parent.AppendChild($node)
    return $node
}

function Get-ChaserRuntime([System.Xml.XmlNode]$Chaser) {
    $runtime = 0
    foreach ($step in @($Chaser.Step)) { $runtime += [int]$step.FadeIn + [int]$step.Hold + [int]$step.FadeOut }
    return $runtime
}

function Add-Show([System.Xml.XmlNode]$Engine, [int]$Id, [string]$Name, [object[]]$Tracks) {
    $show = Add-Element $Engine 'Function' @{ ID=$Id; Type='Show'; Name=$Name }
    [void](Add-Element $show 'TimeDivision' @{ Type='Time'; BPM=120 })
    for ($i = 0; $i -lt $Tracks.Count; $i++) {
        $track = $Tracks[$i]
        $trackNode = Add-Element $show 'Track' @{ ID=$i; Name=$track.Name; isMute=0 }
        [void](Add-Element $trackNode 'ShowFunction' @{ ID=$track.FunctionId; StartTime=$track.StartTime; Duration=$track.Duration; Color=$track.Color })
    }
}

function Add-ControlPage([System.Xml.XmlNode]$VirtualConsole, [int]$FrameId, [object[]]$Controls) {
    $frame = Add-Element $VirtualConsole 'Frame' @{ Caption='Scene Library - All Universes'; ID=$FrameId }
    $appearance = Add-Element $frame 'Appearance' @{}
    [void](Add-Element $appearance 'Font' @{} 'Roboto Condensed,16,-1,5,400,0,0,0,0,0,0,0,0,0,0,1')
    [void](Add-Element $frame 'WindowState' @{ Visible='True'; X=0; Y=0; Width=1920; Height=1080 })
    [void](Add-Element $frame 'AllowResize' @{} 'False'); [void](Add-Element $frame 'ShowHeader' @{} 'True'); [void](Add-Element $frame 'ShowEnableButton' @{} 'True'); [void](Add-Element $frame 'Collapsed' @{} 'False'); [void](Add-Element $frame 'Disabled' @{} 'False')
    for ($i = 0; $i -lt $Controls.Count; $i++) {
        $column = $i % 6; $row = [math]::Floor($i / 6)
        $control = $Controls[$i]
        $button = Add-Element $frame 'Button' @{ Caption=$control.Caption; ID=$i; Icon='' }
        [void](Add-Element $button 'WindowState' @{ Visible='True'; X=(20 + $column * 310); Y=(40 + $row * 110); Width=290; Height=88 })
        $buttonAppearance = Add-Element $button 'Appearance' @{}
        [void](Add-Element $buttonAppearance 'BackgroundColor' @{} '4282664004')
        [void](Add-Element $button 'Function' @{ ID=$control.Id })
        [void](Add-Element $button 'Action' @{} 'Toggle')
    }
}

if (-not (Test-Path -LiteralPath $SourceWorkspace)) { throw "Source workspace was not found: $SourceWorkspace" }
[xml]$workspace = Get-Content -Raw -LiteralPath $SourceWorkspace
$engine = $workspace.Workspace.Engine
$functions = @($engine.Function)
if ($functions.Count -ne 1114) { throw 'Expected the unmodified v03 library with 1,114 functions.' }

$vibes = @{}
foreach ($function in $functions | Where-Object {$_.Type -eq 'Chaser' -and $_.Name -like 'LIB U* Vibe - *'}) {
    $vibeMatch = [regex]::Match([string]$function.Name, '^LIB U([123]) Vibe - (.+)$')
    if (-not $vibeMatch.Success) { continue }
    $universe = $vibeMatch.Groups[1].Value; $key = $vibeMatch.Groups[2].Value
    if (-not $vibes.ContainsKey($key)) { $vibes[$key] = @{} }
    $vibes[$key][$universe] = $function
}
if ($vibes.Count -ne 51 -or @($vibes.Values | Where-Object {$_.Keys.Count -ne 3}).Count -ne 0) { throw 'Expected 51 complete U1/U2/U3 vibe triplets.' }

$nextId = 1114
$parkAllId = $nextId; $nextId++
Add-Show $engine $parkAllId 'SCENE Park All Universes / Blackout' @(
    [pscustomobject]@{Name='U1 Park'; FunctionId=15; StartTime=0; Duration=1000; Color='#646464'},
    [pscustomobject]@{Name='U2 Park'; FunctionId=398; StartTime=0; Duration=1000; Color='#646464'},
    [pscustomobject]@{Name='U3 Park'; FunctionId=756; StartTime=0; Duration=1000; Color='#646464'}
)

$controls = @([pscustomobject]@{Caption='PARK ALL / BLACKOUT'; Id=$parkAllId})
$catalogRows = @()
foreach ($key in ($vibes.Keys | Sort-Object)) {
    $match = [regex]::Match($key, '^(.*) ([123]): (.*)$')
    if (-not $match.Success) { throw "Unexpected vibe name: $key" }
    $vibe = $match.Groups[1].Value; $variation = [int]$match.Groups[2].Value; $subtitle = $match.Groups[3].Value
    $offset = switch ($variation) { 1 { 500 } 2 { 900 } default { 1300 } }
    $tracks = @()
    foreach ($universe in @('1','2','3')) {
        $chaser = $vibes[$key][$universe]
        $tracks += [pscustomobject]@{
            Name="U$universe $vibe"; FunctionId=[int]$chaser.ID; StartTime=([int]$universe - 1) * $offset;
            Duration=(Get-ChaserRuntime $chaser); Color=if ($universe -eq '1') {'#b04040'} elseif ($universe -eq '2') {'#4080b0'} else {'#40a060'}
        }
    }
    $showId = $nextId; $nextId++
    Add-Show $engine $showId "SCENE $vibe $variation - $subtitle" $tracks
    $controls += [pscustomobject]@{Caption="$vibe $variation - $subtitle"; Id=$showId}
    $endTime = ($tracks | ForEach-Object {$_.StartTime + $_.Duration} | Measure-Object -Maximum).Maximum
    $catalogRows += [pscustomobject]@{Id=$showId; Vibe=$vibe; Variation=$variation; Subtitle=$subtitle; Offset=$offset; Duration=$endTime}
}
Add-ControlPage $workspace.Workspace.VirtualConsole 5 $controls

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$writer = [System.Xml.XmlWriter]::Create($OutputWorkspace, $settings)
$workspace.Save($writer); $writer.Dispose()

$catalogPath = Join-Path (Split-Path -Parent $OutputWorkspace) 'Derek-Animation-Catalog-v04.md'
$mappingPath = Join-Path (Split-Path -Parent $OutputWorkspace) 'Derek-QLC-Mapping-Reference-v04.md'
$catalog = @"
# Derek animation catalog v04 — all-universe scenes

v04 retains all independent U1/U2/U3 libraries and adds 51 all-universe **SCENE** functions. Each is a QLC+ Show with three parallel tracks: U1 begins first, U2 follows after a small offset, and U3 follows after twice that offset. This makes the clusters feel independently active while using the same named vibe and variation. No QLC+ audio functions or cluster audio triggers are included.

Use the `Scene Library - All Universes` Virtual Console page for `PARK ALL / BLACKOUT` and the 51 themed scenes. Do not launch a scene alongside an independent function for U1, U2, or U3, because they own the same channels.

| Scene ID | Vibe | Variation | Name | U2/U3 offset | Total runtime |
| ---: | --- | ---: | --- | ---: | ---: |
"@
foreach ($row in $catalogRows | Sort-Object Vibe,Variation) { $catalog += "| $($row.Id) | $($row.Vibe) | $($row.Variation) | $($row.Subtitle) | $($row.Offset) ms | $([math]::Round($row.Duration / 1000,1)) s |`n" }
$catalog += @"

## Audio handoff

The scene timing above is intentionally independent of audio. Add a QLC+ audio function or a cluster-controller DFPlayer trigger to a fourth Show track when the track numbers and desired cue points are confirmed. Preserve the existing three motion tracks and their stagger offsets.
"@
Set-Content -LiteralPath $catalogPath -Value $catalog -Encoding utf8

@'
# Derek QLC+ mapping reference v04

`SCENE <Vibe> <Variation>` functions are QLC+ Shows, not flat scenes. Each show has one motion-and-light chaser track for U1, U2, and U3. The track functions own their respective controller plus eight Derek fixtures:

| Track | Controller | Derek fixture IDs |
| --- | ---: | --- |
| U1 | 8 | 0-7 |
| U2 | 17 | 9-16 |
| U3 | 19 | 20-27 |

The Show offset is 0.5 s for variation 1, 0.9 s for variation 2, and 1.3 s for variation 3. Each child animation performs its own Park/Blackout ending. `SCENE Park All Universes / Blackout` explicitly parks all three at once.

U2/U3 calibration remains a commissioning requirement: their generated center and physical direction assumptions must be tested before live playback.
'@ | Set-Content -LiteralPath $mappingPath -Encoding utf8

Write-Host "Created $OutputWorkspace"
Write-Host "Created $catalogPath"
Write-Host "Created $mappingPath"
Write-Host "Added $($catalogRows.Count) all-universe scene shows."
