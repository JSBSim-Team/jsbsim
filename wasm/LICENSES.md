# Component licenses

The TypeScript SDK, generator and packaging source derived from
[0x62/jsbsim-wasm](https://github.com/0x62/jsbsim-wasm) retain the MIT license
in [LICENSE](LICENSE), including Benedict Lewis (0x62)'s copyright. See
[NOTICE](NOTICE) for source attribution.

The compiled WebAssembly module includes JSBSim C++ code under its existing
GNU Lesser General Public License terms. This import does not relicense native
code under MIT. JSBSim's license text is in the repository's `COPYING`; individual
native files retain their original headers. The package includes that text in
`dist/licenses/jsbsim/COPYING` and identifies the exact source repository and
commit in `dist/build-metadata.json` and `dist/licenses/README.txt`.

GeographicLib and the bundled Expat XML parser retain the license notices copied
from `src/GeographicLib/LICENSE.txt` and `src/simgear/xml/COPYING` under
`dist/licenses/jsbsim/`. SDK notices are also included under `dist/licenses/`.

Build from the recorded source commit with the commands in README.md to modify
or rebuild the native WebAssembly module and its JavaScript package. No npm
publication or official package ownership is configured by this contribution.
