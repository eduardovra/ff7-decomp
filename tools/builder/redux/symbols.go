package redux

import (
	"debug/elf"
	"fmt"
	"strings"
)

// gcc 2.6.3 is invoked with -g -gcoff, which leaves per-line marker labels
// and compiler stamps in the symbol table. Uploading them would bury the
// real function names in the debugger.
func isCompilerNoise(name string) bool {
	if name == "gcc2_compiled." || name == "__gnu_compiled_c" {
		return true
	}
	if strings.HasPrefix(name, "$L") {
		return true
	}
	if !strings.HasPrefix(name, "LM") {
		return false
	}
	digits := strings.TrimPrefix(name, "LM")
	if digits == "" {
		return false
	}
	return strings.Trim(digits, "0123456789") == ""
}

func isUploadable(sym elf.Symbol) bool {
	switch elf.ST_TYPE(sym.Info) {
	case elf.STT_FUNC, elf.STT_OBJECT, elf.STT_NOTYPE:
	default:
		return false
	}
	if sym.Name == "" || sym.Value == 0 {
		return false
	}
	return !isCompilerNoise(sym.Name)
}

// SymbolMap renders an ELF's symbol table in the format PCSX-Redux accepts:
// one "<hex address> <name>" pair per line, address first.
func SymbolMap(elfPath string) (string, error) {
	f, err := elf.Open(elfPath)
	if err != nil {
		return "", fmt.Errorf("open %s: %w", elfPath, err)
	}
	defer f.Close()

	symbols, err := f.Symbols()
	if err != nil {
		return "", fmt.Errorf("read symbols from %s: %w", elfPath, err)
	}

	var sb strings.Builder
	for _, sym := range symbols {
		if !isUploadable(sym) {
			continue
		}
		fmt.Fprintf(&sb, "%08x %s\n", uint32(sym.Value), sym.Name)
	}
	return sb.String(), nil
}
