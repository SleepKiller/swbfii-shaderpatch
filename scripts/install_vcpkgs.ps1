$Packages = "directxtex", "directxmesh", "openexr", "zlib", "stb", "glm", "ms-gsl", "nlohmann-json", "clara", "yaml-cpp", "freetype"
$x86Packages = $Packages + ("detours")

./vcpkg install --triplet x64-windows $Packages
./vcpkg install --triplet x86-windows $x86Packages
