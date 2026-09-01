package cmd

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/spf13/cobra"
	"github.com/xeeynamo/ff7-decomp/tools/builder/builder"
	"github.com/xeeynamo/ff7-decomp/tools/builder/redux"
)

var (
	reduxHost        string
	reduxSymbolsOnly bool
	reduxNoReset     bool
	reduxClear       bool
)

var reduxCmd = &cobra.Command{
	Use:   "redux <overlay>",
	Short: "Push a built overlay and its symbols into a running PCSX-Redux",
	Long: "Patches the disc image loaded in PCSX-Redux with build/<version>/" +
		"<overlay>.exe and uploads that overlay's symbols to the debugger. " +
		"The patch lives in a PPF layer, so no file under disks/ is modified.",
	Args:          cobra.MaximumNArgs(1),
	SilenceErrors: true,
	SilenceUsage:  true,
	RunE: func(cmd *cobra.Command, args []string) error {
		client := redux.NewClient(reduxHost)
		if reduxClear {
			if err := client.ClearPatches(); err != nil {
				return err
			}
			fmt.Println("disc image restored to its unpatched contents")
			return nil
		}
		if len(args) != 1 {
			return fmt.Errorf("requires the overlay name, e.g. savemenu")
		}
		return pushOverlay(client, args[0])
	},
}

func pushOverlay(client *redux.Client, name string) error {
	b, err := builder.LoadConfig("")
	if err != nil {
		return err
	}
	overlay, err := b.FindOverlay(name)
	if err != nil {
		return err
	}

	if err := client.Pause(); err != nil {
		return err
	}
	if !reduxSymbolsOnly {
		if err := patchOverlay(client, b, overlay); err != nil {
			return err
		}
	}
	if err := uploadSymbols(client, b, overlay); err != nil {
		return err
	}
	if !reduxNoReset {
		if err := client.SoftReset(); err != nil {
			return err
		}
	}
	return client.Resume()
}

func patchOverlay(
	client *redux.Client,
	b builder.BuildConfig,
	overlay builder.Overlay,
) error {
	exePath := filepath.Join(b.BuildPath, overlay.Name+".exe")
	payload, err := redux.Payload(exePath, overlay.DiskPath)
	if err != nil {
		return err
	}
	isoPath, err := redux.IsoPath(overlay.DiskPath)
	if err != nil {
		return err
	}
	if err := client.PatchFile(isoPath, payload); err != nil {
		return err
	}
	fmt.Printf("patched %s with %s (%d bytes)\n", isoPath, exePath, len(payload))
	return nil
}

// main is linked twice; only the _final pass resolves against the overlay
// exports, and it is the one .exe is built from.
func symbolElfPath(b builder.BuildConfig, name string) string {
	finalPath := filepath.Join(b.BuildPath, name+"_final.elf")
	if _, err := os.Stat(finalPath); err == nil {
		return finalPath
	}
	return filepath.Join(b.BuildPath, name+".elf")
}

// Overlays share load addresses -- every menu lives at 0x801D0000 -- so only
// the one being tested is uploaded, alongside the always-resident main.
func uploadSymbols(
	client *redux.Client,
	b builder.BuildConfig,
	overlay builder.Overlay,
) error {
	names := []string{"main"}
	if overlay.Name != "main" {
		names = append(names, overlay.Name)
	}
	if err := client.ResetSymbols(); err != nil {
		return err
	}
	for _, name := range names {
		elfPath := symbolElfPath(b, name)
		symbolMap, err := redux.SymbolMap(elfPath)
		if err != nil {
			return err
		}
		if err := client.UploadSymbols(symbolMap); err != nil {
			return err
		}
		fmt.Printf("uploaded %d symbols from %s\n",
			strings.Count(symbolMap, "\n"), elfPath)
	}
	return nil
}

func init() {
	reduxCmd.Flags().StringVar(&reduxHost, "host", redux.DefaultHost,
		"host:port of the PCSX-Redux web server")
	reduxCmd.Flags().BoolVar(&reduxSymbolsOnly, "symbols-only", false,
		"Upload symbols without patching the disc image")
	reduxCmd.Flags().BoolVar(&reduxNoReset, "no-reset", false,
		"Skip the soft reset, leaving the running game untouched")
	reduxCmd.Flags().BoolVar(&reduxClear, "clear", false,
		"Drop every disc patch and restore the loaded image")
	rootCmd.AddCommand(reduxCmd)
}
