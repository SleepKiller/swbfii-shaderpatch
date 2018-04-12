﻿param([string]$binpath=";" + (Resolve-Path "../bin/").Path)

$old_path = $env:Path

$env:Path += $binpath

# Make sure we have a place to put checksums and munged files.
mkdir -Force build/ > $null
mkdir -Force munged/ > $null

function file_changed($file)
{
	$file_name = $file.Name;

   $checksum_path = "build/$file_name.checksum"

   $result = $true
   $hash = (Get-FileHash -Path $file -Algorithm MD5).hash

   if (Test-Path -Path $checksum_path) 
   {
      $result = ((Get-Content -Raw $checksum_path) -ne $hash)
   }

   $hash | Out-File -NoNewline -Encoding utf8 $checksum_path

   return $result
}

function include_file_changed()
{
   $constants_list = file_changed (Get-Item "src/constants_list.hlsl")
   $ext_constants_list = file_changed (Get-Item "src/ext_constants_list.hlsl")
   $fog_utilities = file_changed (Get-Item "src/fog_utilities.hlsl")
   $lighting_utilities = file_changed (Get-Item "src/lighting_utilities.hlsl")
   $pixel_utilities = file_changed (Get-Item "src/pixel_utilities.hlsl")
   $transform_utilities = file_changed (Get-Item "src/transform_utilities.hlsl")
   $vertex_utilities = file_changed (Get-Item "src/vertex_utilities.hlsl")

   return $constants_list -or $ext_constants_list -or $fog_utilities -or $lighting_utilities -or $pixel_utilities -or $transform_utilities -or $vertex_utilities
}

function source_files_changed($srcname)
{
   $fx_src_changed = file_changed (Get-Item "src\$srcname")
   $hlsl_src_changed = $false

   $hlsl_name = $srcname -replace ".fx", ".hlsl"

   if (Test-Path -Path "src\$hlsl_name")
   {
      $hlsl_src_changed = file_changed (Get-Item "src\$hlsl_name")
   }

   return ($fx_src_changed -or $hlsl_src_changed)
}

function remove_shader_checksums($srcname)
{
   if (Test-Path -Path "src\$srcname")
   {
      Remove-Item -Path "build\$srcname.checksum"
   }
   
   $json_name = $srcname -replace ".fx", ".json"

   if (Test-Path -Path "src\$json_name")
   {
      Remove-Item -Path "build\$json_name.checksum"
   }

   $hlsl_name = $srcname -replace ".fx", ".hlsl"

   if (Test-Path -Path "src\$hlsl_name")
   {
      Remove-Item -Path "build\$hlsl_name.checksum"
   }
}

$munge_all = include_file_changed

foreach ($file in Get-ChildItem -File -Path definitions\* -Include *.json)
{
   $srcname = $file.Name -replace ".json", ".fx"
   $outname = $file.Name -replace ".json", ".shader"

   $definition_changed = file_changed $file
   $source_changed = source_files_changed $srcname
   $munged_exists = Test-Path -Path "munged\$outname"
	
   if ((-not $munge_all) -and (-not $definition_changed) -and (-not $source_changed) -and $munged_exists) { continue }

   Write-Host Munging $file.Name

   shader_compiler $file.FullName "munged\$outname" "src\$srcname" 

   if ($LASTEXITCODE -ne 0)
   {
      remove_shader_checksums $srcname

      throw "Compiling " + $file.Name + " failed!"
   }
}

foreach ($file in Get-ChildItem -File -Path patch_definitions\* -Include *.json)
{
   $outname = $file.Name -replace ".json", ".shader"

   Write-Host Munging $file.Name

   shader_compiler -p $file.FullName "munged\$outname"

   if ($LASTEXITCODE -ne 0)
   {
      remove_shader_checksums $srcname

      throw "Compiling " + $file.Name + " failed!"
   }
}

lvl_pack ".\core.lvldef" "munged\" ".\core.lvl"

$env:Path = $old_path