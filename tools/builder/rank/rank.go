package rank

import (
	"bufio"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"

	"github.com/xeeynamo/ff7-decomp/tools/builder/builder"
)

type entry struct {
	score        float32
	name         string
	instructions int
	branches     int
	jumps        int
	labels       int
}

var includeAsmPattern = regexp.MustCompile(`INCLUDE_ASM\([^,]*,\s*(\w+)\s*\)`)

// Mnemonics fed to the difficulty model. Only the ones the PSX disassembly
// actually emits; jr/jalr are excluded to keep the model's jump feature as
// trained.
var branchMnemonics = map[string]bool{
	"b": true, "beq": true, "bne": true, "bnez": true, "beqz": true,
	"blez": true, "bgtz": true, "bltz": true, "bgez": true,
	"blt": true, "bgt": true, "ble": true, "bge": true,
	"bltzal": true, "bgezal": true,
}

var jumpMnemonics = map[string]bool{"j": true, "jal": true}

func Rank(path string, minThreshold float32, limit int) error {
	requested := path
	pending, err := pendingFunctions(path)
	if err != nil {
		return err
	}
	path = strings.TrimPrefix(path, "src/")
	path = strings.TrimSuffix(path, ".c")
	if !strings.HasPrefix(path, "asm/") {
		path = filepath.Join("asm", builder.Version(), path)
	}
	if _, err := os.Stat(path); os.IsNotExist(err) {
		targetedPath := filepath.Join(filepath.Dir(path), "nonmatchings", filepath.Base(path))
		if _, err := os.Stat(targetedPath); !os.IsNotExist(err) {
			path = targetedPath
		}
	} else {
		targetedPath := filepath.Join(path, "nonmatchings")
		if _, err := os.Stat(targetedPath); !os.IsNotExist(err) {
			path = targetedPath
		}
	}
	var entries []entry
	if err := filepath.Walk(path, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if !strings.Contains(path, "nonmatchings") {
			return nil
		}
		if !strings.HasSuffix(path, ".s") {
			return nil
		}
		funcName := strings.TrimSuffix(filepath.Base(path), ".s")
		if pending != nil && !pending[funcName] {
			return nil
		}
		ranked, err := rankFunction(path)
		if err != nil {
			return err
		}
		if ranked == nil || ranked.score < minThreshold {
			return nil
		}
		entries = append(entries, *ranked)
		return nil
	}); err != nil {
		return err
	}
	sort.Slice(entries, func(i, j int) bool {
		if entries[i].score != entries[j].score {
			return entries[i].score < entries[j].score
		}
		if entries[i].instructions != entries[j].instructions {
			return entries[i].instructions < entries[j].instructions
		}
		return entries[i].name < entries[j].name
	})
	report(entries, requested, limit)
	return nil
}

func report(entries []entry, requested string, limit int) {
	if len(entries) == 0 {
		fmt.Println("no pending functions")
		return
	}
	shown := entries
	if limit > 0 && limit < len(shown) {
		shown = shown[:limit]
	}
	fmt.Printf("%5s  %5s  %3s  %3s  %3s  %s\n",
		"score", "instr", "br", "jmp", "lbl", "function")
	for _, entry := range shown {
		fmt.Printf("%.3f  %5d  %3d  %3d  %3d  %s\n",
			entry.score,
			entry.instructions,
			entry.branches,
			entry.jumps,
			entry.labels,
			entry.name)
	}
	fmt.Printf("\n%d pending", len(entries))
	if len(shown) < len(entries) {
		fmt.Printf(", showing %d", len(shown))
	}
	fmt.Println()
	printNextCommands(shown[0].name, overlayName(requested))
}

// printNextCommands echoes the decompile and diff invocations for the
// easiest function, ready to paste.
func printNextCommands(funcName, overlay string) {
	fmt.Printf("  ./mako.sh dec %s\n", funcName)
	differ := ".venv/bin/python3 tools/asm-differ/diff.py -mowsc --annotate " + funcName
	if overlay != "" {
		differ += " --overlay " + overlay
	}
	fmt.Printf("  %s\n", differ)
}

// overlayName maps the ranked path to an overlay the differ accepts, which it
// validates by the presence of a linker map. An empty result drops the flag.
func overlayName(requested string) string {
	trimmed := strings.TrimSuffix(requested, "/")
	candidate := strings.TrimSuffix(filepath.Base(trimmed), ".c")
	if hasLinkerMap(candidate) {
		return candidate
	}
	parent := filepath.Base(filepath.Dir(trimmed))
	if hasLinkerMap(parent) {
		return parent
	}
	return ""
}

func hasLinkerMap(overlay string) bool {
	if overlay == "" || overlay == "." || overlay == string(filepath.Separator) {
		return false
	}
	mapPath := filepath.Join("build", builder.Version(), overlay+".map")
	_, err := os.Stat(mapPath)
	return err == nil
}

// pendingFunctions lists the functions still stubbed with INCLUDE_ASM in the
// C sources for path. A nil result means no source was found, so every
// function gets ranked.
func pendingFunctions(path string) (map[string]bool, error) {
	sources, err := sourceFiles(path)
	if err != nil || sources == nil {
		return nil, err
	}
	pending := map[string]bool{}
	for _, source := range sources {
		content, err := os.ReadFile(source)
		if err != nil {
			return nil, err
		}
		for _, match := range includeAsmPattern.FindAllStringSubmatch(string(content), -1) {
			pending[match[1]] = true
		}
	}
	return pending, nil
}

func sourceFiles(path string) ([]string, error) {
	if strings.HasPrefix(path, "asm/") {
		return nil, nil
	}
	if !strings.HasPrefix(path, "src/") {
		path = filepath.Join("src", path)
	}
	info, err := os.Stat(path)
	if os.IsNotExist(err) {
		return nil, nil
	}
	if err != nil {
		return nil, err
	}
	if info.IsDir() {
		return filepath.Glob(filepath.Join(path, "*.c"))
	}
	return []string{path}, nil
}

// NOTE: decompilationDifficultyScore and rankFunction are directly converted
// from https://github.com/cdlewis/snowboardkids2-decomp/blob/main/CLAUDE.md
// all rights reserved to the original author https://github.com/cdlewis
// Read more in this article from the same author:
// https://blog.chrislewis.au/the-unexpected-effectiveness-of-one-shot-decompilation-with-claude/

// decompilationDifficultyScore calculates the ML-based difficulty score
// Based on the Python implementation's logistic regression model
func decompilationDifficultyScore(instructions, branches, jumps, labels int) float32 {
	// Standardization parameters (from training)
	means := []float64{34.27065527065527, 1.6666666666666667, 3.1880341880341883, 1.98005698005698}
	stds := []float64{24.763225638334454, 2.047860394102145, 3.200600790997309, 2.3803926026229827}

	// Model coefficients
	coefficients := []float64{2.499706543629367, -0.46648920346754463, -1.61606926820365, 0.4911494991317799}
	intercept := -0.5155412977000488

	// Calculate score
	features := []float64{float64(instructions), float64(branches), float64(jumps), float64(labels)}

	// Standardize features
	logit := intercept
	for i := 0; i < 4; i++ {
		featuresScaled := (features[i] - means[i]) / stds[i]
		logit += featuresScaled * coefficients[i]
	}

	// Sigmoid function
	difficulty := 1.0 / (1.0 + math.Exp(-logit))
	return float32(difficulty)
}

// rankFunction scores one disassembly file, returning nil for the data, bss
// and rodata dumps that share the directory with real functions.
func rankFunction(path string) (*entry, error) {
	// Extract function name from file
	filename := filepath.Base(path)
	funcName := strings.TrimSuffix(filename, ".s")

	// Skip non-code sections (data, bss, rodata, header)
	if strings.HasPrefix(funcName, "jtbl_") ||
		strings.HasPrefix(funcName, "D_") ||
		strings.HasSuffix(funcName, ".data") ||
		strings.HasSuffix(funcName, ".bss") ||
		strings.HasSuffix(funcName, ".rodata") ||
		funcName == "header" {
		return nil, nil
	}

	// Open and read file
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	labelPattern := regexp.MustCompile(`^\s*\.L[0-9A-Fa-f_]+:`)
	instructionPattern := regexp.MustCompile(`/\*\s*[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s*\*/`)

	instructionCount := 0
	branchCount := 0
	jumpCount := 0
	labelCount := 0

	// Read file content to check for rodata-only files
	scanner := bufio.NewScanner(file)
	var content strings.Builder
	for scanner.Scan() {
		content.WriteString(scanner.Text())
		content.WriteString("\n")
	}
	if err := scanner.Err(); err != nil {
		return nil, err
	}

	contentStr := content.String()

	// Skip files that only contain rodata (no actual code)
	if strings.Contains(contentStr, ".section .rodata") && !strings.Contains(contentStr, "glabel") {
		return nil, nil
	}

	// Parse line by line
	scanner = bufio.NewScanner(strings.NewReader(contentStr))
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())

		// Skip empty lines, glabel, endlabel, and comments
		if line == "" ||
			strings.HasPrefix(line, "glabel") ||
			strings.HasPrefix(line, "endlabel") ||
			strings.HasPrefix(line, "nonmatching") {
			continue
		}

		// Count local labels
		if labelPattern.MatchString(line) {
			labelCount++
			continue
		}

		// Count instructions
		if instructionPattern.MatchString(line) {
			opcode := mnemonic(line)
			// Data dumps carry the same address comment as code, so reject
			// the assembler directives that make up their bodies.
			if opcode == "" || strings.HasPrefix(opcode, ".") {
				continue
			}
			instructionCount++

			switch {
			case branchMnemonics[opcode]:
				branchCount++
			case jumpMnemonics[opcode]:
				jumpCount++
			}
		}
	}

	if err := scanner.Err(); err != nil {
		return nil, err
	}

	// Skip functions with 0 instructions (data sections)
	if instructionCount == 0 {
		return nil, nil
	}

	score := decompilationDifficultyScore(instructionCount, branchCount, jumpCount, labelCount)
	return &entry{
		score:        score,
		name:         funcName,
		instructions: instructionCount,
		branches:     branchCount,
		jumps:        jumpCount,
		labels:       labelCount,
	}, nil
}

// mnemonic returns the opcode of a disassembled line, which follows the
// address comment. Matching it there avoids hitting the encoded bytes or an
// operand that happens to spell an opcode.
func mnemonic(line string) string {
	index := strings.Index(line, "*/")
	if index < 0 {
		return ""
	}
	fields := strings.Fields(line[index+2:])
	if len(fields) == 0 {
		return ""
	}
	return fields[0]
}
