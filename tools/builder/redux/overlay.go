package redux

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
	"os/exec"
	"strings"
)

// The compressed overlays are inflated to .dec files as diff targets, so a
// disk_path ending in .dec marks an overlay that has to be re-wrapped.
const decSuffix = ".dec"

const overlayHeaderSize = 8

// IsoPath turns a config disk_path into the name PCSX-Redux's ISO reader
// expects. That reader compares directory records verbatim, so the ";1"
// version suffix is mandatory.
func IsoPath(diskPath string) (string, error) {
	path := strings.TrimSuffix(diskPath, decSuffix)
	parts := strings.SplitN(path, "/", 3)
	if len(parts) != 3 || parts[0] != "disks" {
		return "", fmt.Errorf("cannot derive an ISO path from %q", diskPath)
	}
	return parts[2] + ";1", nil
}

// Payload is the byte sequence to write over the on-disc file: the raw .exe
// when the overlay is stored uncompressed, or a gzip stream behind the
// game's 8-byte header when it is not.
func Payload(exePath, diskPath string) ([]byte, error) {
	exe, err := os.ReadFile(exePath)
	if err != nil {
		return nil, fmt.Errorf("read %s: %w", exePath, err)
	}
	onDiscPath := strings.TrimSuffix(diskPath, decSuffix)
	onDisc, err := os.ReadFile(onDiscPath)
	if err != nil {
		return nil, fmt.Errorf("read %s: %w", onDiscPath, err)
	}

	payload := exe
	if strings.HasSuffix(diskPath, decSuffix) {
		payload, err = wrapCompressed(exe, onDisc)
		if err != nil {
			return nil, fmt.Errorf("%s: %w", onDiscPath, err)
		}
	}
	if len(payload) > len(onDisc) {
		return nil, fmt.Errorf(
			"%s: %d bytes do not fit the %d the file occupies on disc",
			onDiscPath,
			len(payload),
			len(onDisc),
		)
	}
	return payload, nil
}

func wrapCompressed(exe, onDisc []byte) ([]byte, error) {
	if len(onDisc) < overlayHeaderSize {
		return nil, fmt.Errorf("file is too short to carry an overlay header")
	}

	var out bytes.Buffer
	size := uint32(len(exe))
	if err := binary.Write(&out, binary.LittleEndian, size); err != nil {
		return nil, err
	}
	// The header's second word follows from no size we can compute, so
	// carry the original's through untouched.
	out.Write(onDisc[4:overlayHeaderSize])

	// Go's compress/gzip lands about 2% larger than the original streams,
	// which is enough to overflow the space every compressed overlay has on
	// disc. -n keeps the FLG byte zero, the one header field Unzip() in
	// src/main/unzip.c insists on; it skips the rest of the header.
	cmd := exec.Command("gzip", "-9", "-n", "-c")
	cmd.Stdin = bytes.NewReader(exe)
	cmd.Stdout = &out
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return nil, fmt.Errorf("gzip: %w", err)
	}
	return out.Bytes(), nil
}
