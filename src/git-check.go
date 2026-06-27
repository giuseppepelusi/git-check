package main

import (
	"bufio"
	"flag"
	"fmt"
	"io/fs"
	"os"
	"os/exec"
	"os/user"
	"path/filepath"
	"strconv"
	"strings"
)

const (
	colorGreen  = "\033[32m"
	colorRed    = "\033[31m"
	colorYellow = "\033[33m"
	colorBold   = "\033[1m"
	colorReset  = "\033[0m"
)

var noColor = os.Getenv("NO_COLOR") != ""

func colorize(code, text string) string {
	if noColor {
		return text
	}
	return code + text + colorReset
}

type RepoStatus struct {
	Path        string
	Branch      string
	Dirty       bool
	HasUpstream bool
	Ahead       int
	Behind      int
	FetchErr    error
}

func main() {
	var showBranch, showPath, showHelp bool
	var directory string

	flag.BoolVar(&showBranch, "b", false, "show current branch for each repository")
	flag.BoolVar(&showBranch, "branch", false, "show current branch for each repository")
	flag.BoolVar(&showPath, "p", false, "show full path instead of just directory name")
	flag.BoolVar(&showPath, "path", false, "show full path instead of just directory name")
	flag.StringVar(&directory, "d", "", "directory to scan (default: current directory)")
	flag.StringVar(&directory, "directory", "", "directory to scan (default: current directory)")
	flag.BoolVar(&showHelp, "h", false, "show this help message")
	flag.BoolVar(&showHelp, "help", false, "show this help message")

	flag.Usage = printHelp
	flag.Parse()

	if showHelp {
		printHelp()
		os.Exit(0)
	}

	if flag.NArg() > 0 {
		fmt.Fprintf(os.Stderr, "git-check: unexpected argument: %q\n", flag.Arg(0))
		printHelp()
		os.Exit(1)
	}

	var err error
	if directory == "" {
		directory, err = os.Getwd()
	} else {
		directory, err = filepath.Abs(directory)
	}
	if err != nil {
		fmt.Fprintln(os.Stderr, "git-check:", err)
		os.Exit(1)
	}

	repos, err := findRepos(directory)
	if err != nil {
		fmt.Fprintln(os.Stderr, "git-check:", err)
		os.Exit(1)
	}

	home := homeDir()

	for _, repo := range repos {
		status := checkRepo(repo)
		printStatus(status, showBranch, showPath, home)
	}
}

func printHelp() {
	fmt.Fprintln(os.Stderr, "usage: git check [<options>]")
	fmt.Fprintln(os.Stderr)
	fmt.Fprintln(os.Stderr, "    -b, --branch          show current branch for each repository")
	fmt.Fprintln(os.Stderr, "    -p, --path            show full path instead of just directory name")
	fmt.Fprintln(os.Stderr, "    -d, --directory <dir> directory to scan (default: current directory)")
	fmt.Fprintln(os.Stderr, "    -h, --help            show this help message")
}

func findRepos(base string) ([]string, error) {
	var repos []string

	err := filepath.WalkDir(base, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			fmt.Fprintln(os.Stderr, "git-check:", err)
			return nil
		}
		if !d.IsDir() {
			return nil
		}
		if d.Name() == ".git" {
			return fs.SkipDir
		}
		if isGitRepo(path) {
			repos = append(repos, path)
		}
		return nil
	})

	return repos, err
}

func isGitRepo(path string) bool {
	_, err := os.Lstat(filepath.Join(path, ".git"))
	return err == nil
}

func checkRepo(path string) RepoStatus {
	status := RepoStatus{Path: path}

	fetchCmd := exec.Command("git", "-C", path, "fetch", "--quiet")
	fetchCmd.Env = append(os.Environ(), "GIT_TERMINAL_PROMPT=0")
	if err := fetchCmd.Run(); err != nil {
		status.FetchErr = err
	}

	out, err := exec.Command("git", "-C", path, "status", "--porcelain=v2", "--branch").Output()
	if err != nil {
		status.Branch = "?"
		return status
	}

	scanner := bufio.NewScanner(strings.NewReader(string(out)))
	for scanner.Scan() {
		line := scanner.Text()
		switch {
		case strings.HasPrefix(line, "# branch.head "):
			status.Branch = strings.TrimPrefix(line, "# branch.head ")
		case strings.HasPrefix(line, "# branch.upstream "):
			status.HasUpstream = true
		case strings.HasPrefix(line, "# branch.ab "):
			for _, f := range strings.Fields(line) {
				switch {
				case strings.HasPrefix(f, "+"):
					status.Ahead, _ = strconv.Atoi(strings.TrimPrefix(f, "+"))
				case strings.HasPrefix(f, "-"):
					status.Behind, _ = strconv.Atoi(strings.TrimPrefix(f, "-"))
				}
			}
		case strings.HasPrefix(line, "#"):
		default:
			status.Dirty = true
		}
	}

	return status
}

func printStatus(s RepoStatus, showBranch, showPath bool, home string) {
	label := filepath.Base(s.Path)
	if showPath {
		label = displayPath(s.Path, home)
	}
	fmt.Printf("%s: ", colorize(colorBold, label))

	if showBranch {
		branch := s.Branch
		if branch == "" {
			branch = "?"
		}
		fmt.Printf("%s ", colorize(colorYellow+colorBold, "<"+branch+">"))
	}

	if s.Dirty {
		printTag("Changes", colorRed)
	} else {
		printTag("Clean", colorGreen)
	}

	switch {
	case !s.HasUpstream:
		printTag("No Remote", colorYellow)
	case s.FetchErr != nil:
		printTag("Offline", colorYellow)
	case s.Ahead > 0:
		printTag("Unpushed", colorRed)
	default:
		printTag("Synced", colorGreen)
	}

	switch {
	case !s.HasUpstream:
		printTag("No Remote", colorYellow)
	case s.FetchErr != nil:
		printTag("Offline", colorYellow)
	case s.Behind > 0:
		printTag("Behind", colorRed)
	default:
		printTag("Updated", colorGreen)
	}

	fmt.Println()
}

func printTag(text, color string) {
	fmt.Printf("[%s] ", colorize(color+colorBold, text))
}

func homeDir() string {
	if h := os.Getenv("HOME"); h != "" {
		return h
	}
	if u, err := user.Current(); err == nil {
		return u.HomeDir
	}
	return ""
}

func displayPath(path, home string) string {
	if home == "" {
		return path
	}
	if path == home {
		return "~"
	}
	if strings.HasPrefix(path, home+string(filepath.Separator)) {
		return "~" + strings.TrimPrefix(path, home)
	}
	return path
}
