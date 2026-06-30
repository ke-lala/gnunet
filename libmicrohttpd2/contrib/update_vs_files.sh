#!/bin/bash

# This file works with bash only

set -e

prj_files='libmicrohttpd2-files.vcxproj'
prj_filtrs='libmicrohttpd2-filters.vcxproj'

echo -n "Updating $prj_files and $prj_filtrs"

echo -ne '\xEF\xBB\xBF' > "$prj_files" # Output BOM
echo '<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">' >> "$prj_files"
echo -ne '\xEF\xBB\xBF' > "$prj_filtrs" # Output BOM
echo '<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
    <Filter Include="Source Files">
      <UniqueIdentifier>{4FC737F1-C7A5-4376-A066-2A32D752A2FF}</UniqueIdentifier>
      <Extensions>cpp;c;cc;cxx;def;odl;idl;hpj;bat;asm;asmx</Extensions>
    </Filter>
    <Filter Include="Internal Headers">
      <UniqueIdentifier>{93995380-89BD-4b04-88EB-625FBE52EBFB}</UniqueIdentifier>
      <Extensions>h;hh;hpp;hxx;hm;inl;inc;xsd</Extensions>
    </Filter>
    <Filter Include="Public Headers">
      <UniqueIdentifier>{ec88d605-3383-4989-8e25-bc8efa824720}</UniqueIdentifier>
      <Extensions>h;hh;hpp;hxx;hm;inl;inc;xsd</Extensions>
    </Filter>
    <Filter Include="Resource Files">
      <UniqueIdentifier>{67DA6AB6-F800-4c08-8B7A-83BB121AAD01}</UniqueIdentifier>
      <Extensions>rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;tiff;tif;png;wav;mfcribbon-ms</Extensions>
    </Filter>
    <Filter Include="Template Files">
      <UniqueIdentifier>{df5ad836-e372-437b-a0e3-299d3675d6b4}</UniqueIdentifier>
      <Extensions>in</Extensions>
    </Filter>
  </ItemGroup>' >> "$prj_filtrs"
echo '  <ItemGroup>
    <ClInclude Include="$(MhdSrc)include\microhttpd2.h" />
    <ClInclude Include="$(MhdSrc)include\microhttpd2_portability.h" />
    <ClInclude Include="$(MhdSrc)incl_priv\mhd_sys_options.h" />
    <ClInclude Include="$(MhdW32Common)mhd_config.h" />' >> "$prj_files"
echo '  <ItemGroup>
    <ClInclude Include="$(MhdSrc)include\microhttpd2.h">
      <Filter>Public Headers</Filter>
    </ClInclude>
    <ClInclude Include="$(MhdSrc)include\microhttpd2_portability.h">
      <Filter>Public Headers</Filter>
    </ClInclude>
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="$(MhdSrc)incl_priv\mhd_sys_options.h">
      <Filter>Internal Headers</Filter>
    </ClInclude>
    <ClInclude Include="$(MhdW32Common)mhd_config.h">
      <Filter>Internal Headers</Filter>
    </ClInclude>' >> "$prj_filtrs"

for src_file in "${@}"; do
  case $src_file in
    *.h )
       case $src_file in
         md5_ext.?|sha*_ext.? ) ;; # skip files
         tls_*.?|*_tls_*.? ) ;; # TLS is not supported for VS builds
         * )
           echo '    <ClInclude Include="$(MhdSrc)mhd2\'"$src_file"'" />' >> "$prj_files"
           echo '    <ClInclude Include="$(MhdSrc)mhd2\'"$src_file"'">
      <Filter>Internal Headers</Filter>
    </ClInclude>' >> "$prj_filtrs"
       esac
  esac
  echo -n '.'
done

echo '  </ItemGroup>
  <ItemGroup>' >> "$prj_files"
echo '  </ItemGroup>
  <ItemGroup>' >> "$prj_filtrs"

for src_file in "${@}"; do
  case $src_file in
    *.c )
       case $src_file in
         md5_ext.?|sha*_ext.? ) ;; # skip files
         tls_*.?|*_tls_*.? ) ;; # TLS is not supported for VS builds
         * )
           echo '    <ClCompile Include="$(MhdSrc)mhd2\'"$src_file"'" />' >> "$prj_files"
           echo '    <ClCompile Include="$(MhdSrc)mhd2\'"$src_file"'">
      <Filter>Source Files</Filter>
    </ClCompile>' >> "$prj_filtrs"
       esac
  esac
  echo -n '.'
done

echo '  </ItemGroup>
  <ItemGroup>
    <ResourceCompile Include="$(MhdW32Common)microhttpd2.rc" />
  </ItemGroup>
  <ItemGroup>
    <CustomBuild Include="$(MhdW32Common)microhttpd2.rc.in">
      <FileType>Document</FileType>
      <Command>PowerShell.exe -Version 3.0 -NonInteractive -NoProfile -ExecutionPolicy Bypass -File "$(MhdW32Common)gen_dll_res.ps1" -BasePath "$(MhdW32Common)\"</Command>
      <Message>Generating .dll description resource</Message>
      <Outputs>$(MhdW32Common)microhttpd2.rc</Outputs>
      <AdditionalInputs>$(MhdW32Common)gen_dll_res.ps1;$(MhdRoot)configure.ac</AdditionalInputs>
    </CustomBuild>
  </ItemGroup>
  <ItemGroup>
    <None Include="$(MhdSrc)mhd2\cpp.hint" />
  </ItemGroup>
</Project>' >> "$prj_files"
echo '  </ItemGroup>
  <ItemGroup>
    <ResourceCompile Include="$(MhdW32Common)microhttpd2.rc">
      <Filter>Resource Files</Filter>
    </ResourceCompile>
  </ItemGroup>
  <ItemGroup>
    <CustomBuild Include="$(MhdW32Common)microhttpd2.rc.in">
      <Filter>Template Files</Filter>
    </CustomBuild>
  </ItemGroup>
  <ItemGroup>
    <None Include="$(MhdSrc)mhd2\cpp.hint" />
  </ItemGroup>
</Project>' >> "$prj_filtrs"

echo " Done"

if command -v unix2dos >/dev/null 2>&1; then
  unix2dos -e -v "$prj_files" "$prj_filtrs"
else
  echo "No unix2dos command. Output files are left with LF line endings."
fi
