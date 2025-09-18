## **A Git extension to overview the status of multiple Git repositories**

git-check is a Git extension that integrates with the git command.
It recursively scans directories and reports the status of all Git repositories it finds.
It helps you quickly identify local changes, unpushed commits, and updates from remote repositories, giving you a concise **overview** of your projects.

Once installed, you can use it as a native git command with `git check` and access its manual page with `git help check`.

### **Install**

To install **git-check**, you can use the quick install script:

```sh
curl -fsSL https://raw.githubusercontent.com/giuseppepelusi/git-check/refs/heads/main/install.sh | bash
```

Or follow these steps to install manually:

1. **Clone the repository:**
   ```sh
   git clone https://github.com/giuseppepelusi/git-check.git
   ```

   or

   ```sh
   git clone git@github.com:giuseppepelusi/git-check.git
   ```

2. **Move to the repository directory:**
    ```sh
    cd git-check
    ```

3. **Run the installer script:**
    ```sh
    make install
    ```

### **Usage**

After installing, you can use the `git check` command as follows:

- **Run a check starting from the current directory:**
    ```sh
    git check
    ```

- **Show the current branch for each repository:**
    ```sh
    git check -b
    git check --branch
    ```

- **Show the full path instead of just the directory name:**
    ```sh
    git check -p
    git check --path
    ```

- **Specify a different directory to start scanning from:**
    ```sh
    git check -d <directory>
    git check --directory <directory>
    ```

- **Combine multiple flags as needed:**
    ```sh
    git check -bpd ~/Projects
    ```

- **Show the list of available commands:**
    ```sh
    git check -h
    ```

- **Access the manual page:**
    ```sh
    git check --help
    git help check
    ```

### **Uninstall**

To uninstall **git-check**:

- **Run the uninstaller script:**
    ```sh
    make uninstall
    ```

- **Or manually remove the files:**
    ```sh
    sudo rm -f /usr/local/bin/git-check
    sudo rm -f /usr/local/man/man1/git-check.1
    ```
