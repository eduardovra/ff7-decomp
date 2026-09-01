package builder

import (
	"crypto/md5"
	"fmt"
	"os"
	"path/filepath"

	"github.com/goccy/go-yaml"
)

type Overlay struct {
	Name                     string   `yaml:"name"`
	DiskPath                 string   `yaml:"disk_path"`
	Compression              string   `yaml:"compression"`
	Sha1                     string   `yaml:"sha1"`
	Sha1Decompressed         string   `yaml:"sha1_decompressed"`
	BasePath                 string   `yaml:"base_path"`
	SymbolAddrsPath          []string `yaml:"symbol_addrs_path"`
	MigrateRodataToFunctions bool     `yaml:"migrate_rodata_to_functions"`
	VramStart                int64    `yaml:"vram_start"`
	GPValue                  int64    `yaml:"gp_value"`
	BssSize                  int64    `yaml:"bss_size"`
	Segments                 [][]any  `yaml:"segments"`
}

type BuildConfig struct {
	Version          string    `yaml:"version"`
	BuildPath        string    `yaml:"build_path"`
	AsmPath          string    `yaml:"asm_path"`
	AssetPath        string    `yaml:"asset_path"`
	SrcPath          string    `yaml:"src_path"`
	LdScriptPath     string    `yaml:"ld_script_path"`
	GeneratedSymPath string    `yaml:"generated_sym_path"`
	Align            int       `yaml:"align"`
	Overlays         []Overlay `yaml:"overlays"`
}

func (o Overlay) Fingerprint() []byte {
	h := md5.New()
	fmt.Fprintf(h, "%v", o)
	// hash the symbol files' contents, not just their paths, so a
	// symbol rename invalidates the overlay's split asm
	for _, path := range o.SymbolAddrsPath {
		if data, err := os.ReadFile(path); err == nil {
			h.Write(data)
		}
	}
	return h.Sum(nil)
}

func (b BuildConfig) FindOverlay(name string) (Overlay, error) {
	for _, o := range b.Overlays {
		if o.Name == name {
			return o, nil
		}
	}
	return Overlay{}, fmt.Errorf("unknown overlay %q", name)
}

func ConfigPath(version string) string {
	if version == "" {
		version = Version()
	}
	return filepath.Join("config", version+".yaml")
}

func LoadConfig(version string) (BuildConfig, error) {
	path := ConfigPath(version)
	data, err := os.ReadFile(path)
	if err != nil {
		return BuildConfig{}, fmt.Errorf("read %s: %w", path, err)
	}
	var b BuildConfig
	if err := yaml.Unmarshal(data, &b); err != nil {
		return BuildConfig{}, fmt.Errorf("parse %s: %w", path, err)
	}
	return b, nil
}
