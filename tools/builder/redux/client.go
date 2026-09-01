// Package redux talks to a running PCSX-Redux through its REST API, so a
// freshly built overlay and its symbols can be pushed into the emulator
// without touching any disc image on disk.
package redux

import (
	"bytes"
	"fmt"
	"io"
	"net/http"
	"time"
)

const DefaultHost = "localhost:8080"

type Client struct {
	host string
	http *http.Client
}

func NewClient(host string) *Client {
	return &Client{
		host: host,
		http: &http.Client{Timeout: 30 * time.Second},
	}
}

// query is appended raw: PCSX-Redux compares ISO filenames verbatim, so
// percent-encoding the '/' and ';1' of a path would make it unfindable.
func (c *Client) post(path, query string, body []byte) error {
	url := fmt.Sprintf("http://%s/api/v1/%s?%s", c.host, path, query)
	resp, err := c.http.Post(
		url, "application/octet-stream", bytes.NewReader(body))
	if err != nil {
		return fmt.Errorf("%s: %w (is Enable Web Server on?)", url, err)
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		detail, _ := io.ReadAll(resp.Body)
		return fmt.Errorf("%s: %s: %s", url, resp.Status, bytes.TrimSpace(detail))
	}
	return nil
}

func (c *Client) Pause() error {
	return c.post("execution-flow", "function=pause", nil)
}

func (c *Client) Resume() error {
	return c.post("execution-flow", "function=resume", nil)
}

func (c *Client) SoftReset() error {
	return c.post("execution-flow", "function=reset&type=soft", nil)
}

// PatchFile overwrites a file in the loaded disc image. The change lives in
// a PPF layer, so the .bin on disk is never modified.
func (c *Client) PatchFile(isoPath string, data []byte) error {
	return c.post("cd/patch", "filename="+isoPath, data)
}

func (c *Client) ClearPatches() error {
	return c.post("cd/ppf", "function=clear", nil)
}

func (c *Client) ResetSymbols() error {
	return c.post("assembly/symbols", "function=reset", nil)
}

func (c *Client) UploadSymbols(symbolMap string) error {
	return c.post("assembly/symbols", "function=upload", []byte(symbolMap))
}
