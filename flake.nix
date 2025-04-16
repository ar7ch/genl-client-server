{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs =
    { self, nixpkgs }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forEachSupportedSystem =
        f:
        nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            pkgs = import nixpkgs { inherit system; };
          }
        );
    in
    {
      devShells = forEachSupportedSystem (
        { pkgs }:
        {
          default =
            pkgs.mkShell.override
              {
                # Override stdenv in order to change compiler:
                # stdenv = pkgs.clangStdenv;
              }
              {
                packages = with pkgs; [
                  clang-tools
                  cmake
                  codespell
                  conan
                  cppcheck
                  doxygen
                  gtest
                  lcov
                  libnl
                  pkg-config
                ];
              };
          shellHook = ''
            echo "Loading dev flake..."
            # export CMAKE_INCLUDE_PATH+=$NIX_CFLAGS_COMPILE
            echo "Dev flake loaded"
          '';
        }
      );
    };
}
