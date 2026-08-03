# Third-Party Licenses

This document lists the third-party libraries used by Velvox Music and their
respective licenses. The corresponding license texts are included in this
package or linked below.

## Qt 6

- **Usage**: GUI framework, QML engine, SQL plugin, shader tools, effects.
- **License**: GNU Lesser General Public License (LGPL) v3 / Commercial
- **Source**: <https://www.qt.io/licensing>
- Qt is dynamically linked. Prebuilt Qt binaries are used under the LGPL or a
  valid commercial license from The Qt Company.

## miniaudio

- **Usage**: Audio decoding and playback.
- **License**: Public domain / Unlicense OR MIT-0 (dual-licensed by the author)
- **Source**: <https://github.com/mackron/miniaudio>
- This library is compiled directly into the application and is safe for
  commercial distribution.

## TagLib

- **Usage**: Audio metadata parsing (ID3, Vorbis comments, MP4, APE, ...).
- **License**: GNU Lesser General Public License (LGPL) v2.1 / Mozilla Public
  License (MPL) v1.1 (dual-licensed)
- **Source**: <https://github.com/taglib/taglib>
- Velvox Music links to TagLib **dynamically**. The TagLib shared library
  (`tag.dll` / `taglib.dll`) is shipped alongside the executable so that users
  can replace it with a modified version, satisfying the LGPL dynamic-linking
  requirements.

## kissfft

- **Usage**: Fast Fourier Transform for realtime spectrum analysis.
- **License**: BSD-3-Clause
- **Source**: <https://github.com/mborgerding/kissfft>
- This library is compiled directly into the application.

---

## LGPL Source Code Offer

Velvox Music dynamically links against Qt and TagLib, both of which are
available under the GNU Lesser General Public License (LGPL). If you received
this program as a binary distribution and would like a copy of the complete
corresponding source code for the exact versions of Qt and TagLib used to build
it, please open an issue at:

<https://github.com/VeilCode/VelvoxMusic/issues>

You may also obtain the source directly from the upstream projects:

- Qt: <https://www.qt.io/download-open-source>
- TagLib: <https://github.com/taglib/taglib>

For the full text of the MIT license covering Velvox Music itself, see
`LICENSE`.
