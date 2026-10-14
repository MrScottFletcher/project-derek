param(
    [string]$SourceRoot = 'G:\My Drive\2026_Derek_Project\AudioFiles',
    [string]$CardImageRoot = 'G:\My Drive\2026_Derek_Project\AudioFiles\Derek-DFPlayer-SD-Image-v01',
    [string]$FfmpegPath = 'C:\Users\mrsco\AppData\Local\Microsoft\WinGet\Packages\Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe\ffmpeg-9.0.2-full_build\bin\ffmpeg.exe'
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $SourceRoot)) { throw "Audio source folder not found: $SourceRoot" }
if (-not (Test-Path -LiteralPath $FfmpegPath)) { throw "FFmpeg not found: $FfmpegPath" }
if (Test-Path -LiteralPath $CardImageRoot) { throw "Refusing to overwrite an existing card image: $CardImageRoot" }

# IDs are append-only. Do not renumber or reuse a retired ID: add future cues
# at the next free number in the manifest and create the matching MP3 filename.
$cues = @(
    [pscustomobject]@{ TrackId=501; CueId='VOX-RICK-001'; Title='Pickle Rick'; Source='RickAndMorty_PickleRick_3 sec.wav'; Tags='quote,rick-and-morty,comic,startle'; DurationMs=3223; Vibe='Chaotic / playful'; Visual='SCENE Chaotic 1 - Mayhem' },
    [pscustomobject]@{ TrackId=502; CueId='VOX-DEREK-001'; Title='Jason, Jason, Jason'; Source='DerekGoodPlace_JasonJasonJason.wav'; Tags='quote,good-place,derek,comic'; DurationMs=6599; Vibe='Playful / happy'; Visual='SCENE Playful 1 - Peekaboo' },
    [pscustomobject]@{ TrackId=503; CueId='VOX-DEREK-002'; Title='Derek long quote'; Source='DerekGoodPlace_Long.wav'; Tags='quote,good-place,derek,long-form'; DurationMs=356125; Vibe='Playful / welcoming'; Visual='SCENE Playful 3 - Goofy Wave (intro only)' },
    [pscustomobject]@{ TrackId=504; CueId='VOX-NEMO-001'; Title='Mine mine mine (short)'; Source='FindingNemo_Mine_01.wav'; Tags='quote,finding-nemo,seagulls,comic'; DurationMs=4858; Vibe='Chaotic / cheering'; Visual='SCENE Chaotic 1 - Mayhem' },
    [pscustomobject]@{ TrackId=505; CueId='VOX-NEMO-002'; Title='Mine mine mine (one minute)'; Source='FindingNemo_Mine_1minute01.wav'; Tags='quote,finding-nemo,seagulls,comic,long-form'; DurationMs=62495; Vibe='Chaotic / cheering'; Visual='SCENE Chaotic 3 - Everything Everywhere (intro only)' },
    [pscustomobject]@{ TrackId=506; CueId='VOX-NEMO-003'; Title='Mine (stinger)'; Source='FindingNemo_Mine_Only01.wav'; Tags='quote,finding-nemo,seagulls,stinger'; DurationMs=443; Vibe='Startle / playful'; Visual='SCENE Chaotic 1 - Mayhem' },
    [pscustomobject]@{ TrackId=507; CueId='VOX-MINIONS-001'; Title='Cow moo giggle'; Source='Minions_CowMooGiigle_29sec.wav'; Tags='quote,minions,comic,long-form'; DurationMs=28594; Vibe='Fun / playful'; Visual='SCENE Fun 3 - Party Peek' },
    [pscustomobject]@{ TrackId=508; CueId='VOX-MINIONS-002'; Title='Giggles to banana'; Source='Minions_GiiglesToBanana_11sec.wav'; Tags='quote,minions,comic'; DurationMs=11247; Vibe='Happy / fun'; Visual='SCENE Happy 1 - Sunrise' },
    [pscustomobject]@{ TrackId=509; CueId='SFX-MINIONS-001'; Title='Minions laughs'; Source='Minions_Laughs_9sec.wav'; Tags='sfx,minions,laughter,comic'; DurationMs=8532; Vibe='Cheering / fun'; Visual='SCENE Cheering 1 - Go Team' },
    [pscustomobject]@{ TrackId=510; CueId='VOX-TOY-001'; Title='A stranger from the outside'; Source='ToyStory_AStrangerFromTheOutside.wav'; Tags='quote,toy-story,alien,ominous'; DurationMs=9177; Vibe='Nervous / reverent'; Visual='SCENE Nervous 2 - Worry Scan' },
    [pscustomobject]@{ TrackId=511; CueId='VOX-TOY-002'; Title='The claw is our master'; Source='ToyStory_TheClawIsOurMaster.wav'; Tags='quote,toy-story,alien,reverent'; DurationMs=10050; Vibe='Reverent / playful'; Visual='SCENE Reverent 2 - Quiet Vigil' }
)

$mp3Root = Join-Path $CardImageRoot 'MP3'
New-Item -ItemType Directory -Path $mp3Root -Force | Out-Null

foreach ($cue in $cues) {
    $sourcePath = Join-Path $SourceRoot $cue.Source
    if (-not (Test-Path -LiteralPath $sourcePath)) { throw "Missing source clip: $sourcePath" }
    $outputPath = Join-Path $mp3Root ('{0:D4}.mp3' -f $cue.TrackId)
    & $FfmpegPath -hide_banner -loglevel error -y -i $sourcePath -vn -ac 1 -ar 44100 -c:a libmp3lame -b:a 128k $outputPath
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $outputPath)) { throw "FFmpeg conversion failed for $($cue.Source)" }
}

$manifestPath = Join-Path $CardImageRoot 'audio-cues.csv'
$cues | Select-Object @{Name='track_id';Expression={$_.TrackId}}, @{Name='cue_id';Expression={$_.CueId}}, @{Name='title';Expression={$_.Title}}, @{Name='tags';Expression={$_.Tags}}, @{Name='source_file';Expression={$_.Source}}, @{Name='hardware_file';Expression={('{0:D4}.mp3' -f $_.TrackId)}}, @{Name='duration_ms';Expression={$_.DurationMs}}, @{Name='vibe';Expression={$_.Vibe}}, @{Name='suggested_visual_show';Expression={$_.Visual}}, @{Name='status';Expression={'active'}}, @{Name='notes';Expression={'Copy this identical image to every DFPlayer SD card.'}} | Export-Csv -LiteralPath $manifestPath -NoTypeInformation -Encoding utf8

Get-ChildItem -LiteralPath $mp3Root -File -Filter '*.mp3' | Get-FileHash -Algorithm SHA256 | Select-Object @{Name='hardware_file';Expression={$_.Path | Split-Path -Leaf}}, Hash | Sort-Object hardware_file | Export-Csv -LiteralPath (Join-Path $CardImageRoot 'SHA256SUMS.csv') -NoTypeInformation -Encoding utf8

@'
# Project Derek DFPlayer SD-card image v01

Copy the complete contents of this folder to the root of every DFPlayer Mini SD card. Every card must contain the same `MP3` directory and identical numbered files.

The catalog is append-only: add a new source clip to the source folder, assign the next unused track ID in `audio-cues.csv`, build a new versioned card image, and never renumber existing tracks.

Files are mono 44.1 kHz, 128 kbps CBR MP3 for predictable DFPlayer playback. The source WAV files remain untouched outside this image.
'@ | Set-Content -LiteralPath (Join-Path $CardImageRoot 'README.md') -Encoding utf8

Write-Output "Created $CardImageRoot"
Write-Output "Converted $($cues.Count) MP3 files."
