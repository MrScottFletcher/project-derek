param(
    [string]$SourceWorkspace = (Join-Path $PSScriptRoot '..\dereks-animation-library-v01.qxw'),
    [string]$OutputWorkspace = (Join-Path $PSScriptRoot '..\dereks-animation-library-v02.qxw')
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

function Add-Chaser([System.Xml.XmlNode]$Engine, [int]$Id, [string]$Name, [object[]]$Steps) {
    $duration = 0
    foreach ($step in $Steps) { $duration += [int]$step.FadeIn + [int]$step.Hold + [int]$step.FadeOut }
    $chaser = Add-Element $Engine 'Function' @{ ID = $Id; Type = 'Chaser'; Name = $Name }
    # PerStep mode below owns actual cue timing. Keep this fallback short so
    # QLC+ never presents the full chaser runtime as an individual step delay.
    [void](Add-Element $chaser 'Speed' @{ FadeIn = 0; FadeOut = 0; Duration = 1000 })
    [void](Add-Element $chaser 'Direction' @{} 'Forward')
    [void](Add-Element $chaser 'RunOrder' @{} 'SingleShot')
    [void](Add-Element $chaser 'SpeedModes' @{ FadeIn = 'PerStep'; FadeOut = 'PerStep'; Duration = 'PerStep' })
    for ($i = 0; $i -lt $Steps.Count; $i++) {
        $step = $Steps[$i]
        [void](Add-Element $chaser 'Step' @{ Number = $i; FadeIn = $step.FadeIn; Hold = $step.Hold; FadeOut = 0 } $step.FunctionId)
    }
    return $duration
}

function Derek-Values([int]$Pan, [int]$Lift, [int[]]$Spotlight, [int[]]$RightEye, [int[]]$LeftEye) {
    return "0,$Pan,1,$Lift,2,$($Spotlight[0]),3,$($Spotlight[1]),4,$($Spotlight[2]),5,$($RightEye[0]),6,$($RightEye[1]),7,$($RightEye[2]),8,$($LeftEye[0]),9,$($LeftEye[1]),10,$($LeftEye[2])"
}

function Controller-Values([int]$Speed) {
    return "0,8,1,255,2,0,3,0,4,0,5,0,6,0,7,0,8,0,9,0,10,0,11,$Speed,12,0,13,0"
}

function Get-Rainbow([int]$Index) {
    $colors = @(@(72,0,0), @(64,28,0), @(40,48,0), @(0,56,10), @(0,24,64), @(24,0,64), @(64,0,36), @(52,0,52))
    return $colors[$Index % $colors.Count]
}

function Add-VibePose([System.Xml.XmlNode]$Engine, [int]$Id, [string]$Name, [string]$Pose, [int[]]$ColorA, [int[]]$ColorB, [string]$LightPattern, [int]$Speed) {
    # Physical U1 orientation is verified: raw DMX 192 = left, 64 = right.
    $centers = @(129,137,140,139,133,133,134,133)
    $values = @{ '8' = (Controller-Values $Speed) }
    for ($i = 0; $i -lt 8; $i++) {
        $pan = $centers[$i]; $lift = 96
        switch ($Pose) {
            'peek' { $lift = 75 }
            'left' { $pan = 192; $lift = 112 }
            'right' { $pan = 64; $lift = 112 }
            'up' { $lift = 128 }
            'scatter' {
                $pan = if (($i % 2) -eq 0) { 192 } else { 64 }
                $lift = @(75,96,112,128,96,112,75,128)[$i]
            }
            'wave' {
                $pan = if (($i % 3) -eq 0) { 192 } elseif (($i % 3) -eq 1) { 64 } else { $centers[$i] }
                $lift = @(75,96,112,128,128,112,96,75)[$i]
            }
            'park' { $lift = 0 }
        }
        $color = $ColorA
        if ($LightPattern -eq 'alternate' -and ($i % 2) -eq 1) { $color = $ColorB }
        if ($LightPattern -eq 'rainbow') { $color = Get-Rainbow $i }
        if ($LightPattern -eq 'sparkle') { $color = if (($i % 3) -eq 0) { $ColorB } else { $ColorA } }
        if ($Pose -eq 'park') { $color = @(0,0,0) }
        $spot = @([math]::Floor($color[0] / 2), [math]::Floor($color[1] / 2), [math]::Floor($color[2] / 2))
        $values[[string]$i] = Derek-Values $pan $lift $spot $color $color
    }
    Add-Scene $Engine $Id $Name $values
}

function Build-VibeVariation([System.Xml.XmlNode]$Engine, [ref]$NextId, [hashtable]$Vibe, [int]$VariationNumber) {
    $name = $Vibe.Names[$VariationNumber - 1]
    $pattern = @('center','left','right','park')
    $fade = @(1600,800,800,1700)
    # A cue's fade plus hold must remain within five seconds: keep the eyes active.
    $hold = @(1200,1200,1200,1200) # 9.7 s
    $lightPattern = 'uniform'
    if ($VariationNumber -eq 2) {
        $pattern = @('peek','left','center','right','center','park')
        $fade = @(1300,700,700,700,700,1700)
        $hold = @(1500,1500,1500,1500,1500,1500) # 14.8 s
        $lightPattern = 'alternate'
    }
    if ($VariationNumber -eq 3) {
        $pattern = @('scatter','left','right','wave','left','right','up','park')
        $fade = @(1500,700,700,800,700,700,1000,1800)
        $hold = @(1800,1800,1800,1800,1800,1800,1800,1800) # 22.3 s
        $lightPattern = if ($Vibe.Key -in @('chaotic','rainbow_fun')) { 'rainbow' } elseif ($Vibe.Key -eq 'sparkle_ponies') { 'sparkle' } else { 'alternate' }
    }
    if ($Vibe.Key -in @('chaotic','rainbow_fun')) { $lightPattern = 'rainbow' }
    if ($Vibe.Key -eq 'sparkle_ponies') { $lightPattern = 'sparkle' }
    $sceneIds = @()
    for ($i = 0; $i -lt $pattern.Count; $i++) {
        $sceneId = $NextId.Value; $NextId.Value++
        Add-VibePose $Engine $sceneId "LIB U1 Vibe Cue - $($Vibe.Label) $VariationNumber.$($i + 1)" $pattern[$i] $Vibe.ColorA $Vibe.ColorB $lightPattern $Vibe.Speed
        $sceneIds += $sceneId
    }
    $steps = @()
    for ($i = 0; $i -lt $sceneIds.Count; $i++) { $steps += [pscustomobject]@{ FunctionId=$sceneIds[$i]; FadeIn=$fade[$i]; Hold=$hold[$i] } }
    $chaserId = $NextId.Value; $NextId.Value++
    $duration = Add-Chaser $Engine $chaserId "LIB U1 Vibe - $($Vibe.Label) ${VariationNumber}: $name" $steps
    return [pscustomobject]@{ Id=$chaserId; Name=$name; Duration=$duration; Pattern=($pattern -join ' -> ') }
}

if (-not (Test-Path -LiteralPath $SourceWorkspace)) { throw "Source workspace was not found: $SourceWorkspace" }
[xml]$workspace = Get-Content -Raw -LiteralPath $SourceWorkspace
$engine = $workspace.Workspace.Engine
$existingIds = @($engine.Function | ForEach-Object {[int]$_.ID})
if (($existingIds | Measure-Object -Maximum).Maximum -ne 40) { throw 'Expected v01 library function IDs through 40. Refusing to overwrite an unknown revision.' }
$nextId = 41

$vibes = @(
    @{Key='angry'; Label='Angry'; Names=@('Glare','Boil','Red Storm'); ColorA=@(72,0,0); ColorB=@(72,16,0); Speed=100},
    @{Key='aggressive'; Label='Aggressive'; Names=@('Challenge','Charge','Dominance'); ColorA=@(80,0,0); ColorB=@(72,72,72); Speed=80},
    @{Key='nervous'; Label='Nervous'; Names=@('Fidget','Worry Scan','Jitter'); ColorA=@(64,38,0); ColorB=@(28,0,56); Speed=100},
    @{Key='happy'; Label='Happy'; Names=@('Sunrise','Bouncy Hello','Bright Smile'); ColorA=@(72,32,0); ColorB=@(56,0,44); Speed=100},
    @{Key='fun'; Label='Fun'; Names=@('Wiggle','Color Chase','Party Peek'); ColorA=@(0,56,56); ColorB=@(64,0,42); Speed=100},
    @{Key='celebration'; Label='Celebration'; Names=@('Gold Rise','Confetti Sweep','Grand Toast'); ColorA=@(72,38,0); ColorB=@(48,0,68); Speed=100},
    @{Key='reverent'; Label='Reverent'; Names=@('Candle Watch','Quiet Vigil','Procession'); ColorA=@(40,22,5); ColorB=@(30,18,48); Speed=190},
    @{Key='sad'; Label='Sad'; Names=@('Blue Sigh','Lonely Look','Rainfall'); ColorA=@(0,12,64); ColorB=@(0,34,64); Speed=190},
    @{Key='welcoming'; Label='Welcoming'; Names=@('Warm Hello','Open House','Come Closer'); ColorA=@(68,30,0); ColorB=@(16,56,10); Speed=170},
    @{Key='chaotic'; Label='Chaotic'; Names=@('Mayhem','Crossfire','Everything Everywhere'); ColorA=@(60,0,40); ColorB=@(0,56,60); Speed=85},
    @{Key='cheering'; Label='Cheering'; Names=@('Go Team','Victory Wave','Roar'); ColorA=@(0,28,72); ColorB=@(72,38,0); Speed=100},
    @{Key='zombie'; Label='Zombie'; Names=@('Crawl','Graveyard Scan','Undead Swarm'); ColorA=@(18,64,0); ColorB=@(48,0,0); Speed=185},
    @{Key='super_hero'; Label='Super Hero'; Names=@('Hero Rise','Sky Patrol','Power Pose'); ColorA=@(0,24,72); ColorB=@(72,0,0); Speed=100},
    @{Key='patriotic'; Label='Patriotic'; Names=@('Stars','Stripes','Finale'); ColorA=@(64,0,0); ColorB=@(0,16,72); Speed=100},
    @{Key='playful'; Label='Playful'; Names=@('Peekaboo','Tease','Goofy Wave'); ColorA=@(64,0,45); ColorB=@(0,60,60); Speed=115},
    @{Key='rainbow_fun'; Label='Rainbow Fun'; Names=@('Rainbow Hello','Prism Parade','Rainbow Riot'); ColorA=@(56,0,42); ColorB=@(0,56,56); Speed=105},
    @{Key='sparkle_ponies'; Label='Sparkle Ponies'; Names=@('Twinkle Trot','Magic Mane','Glitter Gallop'); ColorA=@(64,12,54); ColorB=@(30,20,72); Speed=125}
)

$catalogRows = @()
foreach ($vibe in $vibes) {
    for ($variation = 1; $variation -le 3; $variation++) {
        $result = Build-VibeVariation $engine ([ref]$nextId) $vibe $variation
        $catalogRows += [pscustomobject]@{ Vibe=$vibe.Label; Variation=$variation; Name=$result.Name; Duration=('{0:N1} s' -f ($result.Duration / 1000)); Pattern=$result.Pattern }
    }
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$writer = [System.Xml.XmlWriter]::Create($OutputWorkspace, $settings)
$workspace.Save($writer); $writer.Dispose()

$catalogPath = Join-Path (Split-Path -Parent $OutputWorkspace) 'Derek-Animation-Catalog-v02.md'
$mappingPath = Join-Path (Split-Path -Parent $OutputWorkspace) 'Derek-QLC-Mapping-Reference-v02.md'
$catalog = @"
# Derek animation catalog v02 — U1 vibe library

This revision retains v01 and adds **51 U1-only one-shot vibe animations**: three variations for each requested vibe. Each owns the U1 controller and all eight U1 Derek fixtures; do not layer two motion chasers together. Every animation ends with all eight Dereks hidden, centered, and dark.

All timing is below one minute. Physical U1 direction is calibrated as raw DMX 192 = left and 64 = right. Peak raw lift is 128; the cluster's installed can profile may reduce it further. LED values stay deliberately moderate.

| Vibe | Variation | Function suffix | Duration | Motion sequence |
| --- | ---: | --- | ---: | --- |
"@
foreach ($row in $catalogRows) { $catalog += "| $($row.Vibe) | $($row.Variation) | $($row.Name) | $($row.Duration) | $($row.Pattern) |`n" }
$catalog += @"

## Variation grammar

- **Variation 1** is an approximately 10-second focused statement: center, left, right, park.
- **Variation 2** is an approximately 15-second scan: peek, left, center, right, center, park. It alternates the two vibe colors between neighboring Dereks.
- **Variation 3** is an approximately 22-second ensemble sequence: scattered pose, left/right looks, wave, high pose, park. Chaotic and Rainbow Fun use per-Derek rainbow colors; Sparkle Ponies uses a three-Derek sparkle pattern.

Run `LIB U1 Park / Blackout` before changing between the older pilot functions and this library. Test an entire vibe on U1 before cloning the behavior to another universe.
"@
Set-Content -LiteralPath $catalogPath -Value $catalog -Encoding utf8

$mapping = @"
# Derek QLC+ mapping reference v02

v02 uses the same verified U1 fixture map and conservative travel envelope as v01. The added scenes use controller fixture ID 8 and Derek fixture IDs 0-7 only; U2 and U3 are deliberately untouched.

## Channel ownership

Every v02 vibe cue writes controller channels (active count 8, apply outputs enabled, audio/status/discovery inactive, selected speed) and all 11 channels of each U1 Derek. This gives each chaser a known ending state but means it must not be run concurrently with another function that controls U1 motion or lighting.

## Movement and safety

- Physical U1 left: pan DMX 192; physical U1 right: pan DMX 64; each Derek uses its verified U1 center value.
- Raw lift stays at 75, 96, 112, or 128; firmware and the persisted can profile enforce the final safe limit.
- Slow/medium motion speeds are selected per vibe. Fast snap movement is not used in the new library.
- All 51 chasers end on a park cue: lift 0, calibrated center, LEDs off.

## Expansion path

Create separate U2/U3 instances only after recording their mechanical left/right orientation, centers, and can profiles. Do not copy U1 fixture IDs into those universes. Cross-universe shows should call the per-universe chasers with deliberate offsets after each instance has passed a hardware test.
"@
Set-Content -LiteralPath $mappingPath -Value $mapping -Encoding utf8

Write-Host "Created $OutputWorkspace"
Write-Host "Created $catalogPath"
Write-Host "Created $mappingPath"
Write-Host "Added $($catalogRows.Count) vibe chasers and $($nextId - 41 - $catalogRows.Count) supporting scenes."
