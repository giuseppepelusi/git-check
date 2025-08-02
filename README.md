## **A simple command-line tool to check Git repositories**

Recursively scans directories and checks the Git status of each repository. Helps track uncommitted changes, unpushed commits, and unpulled updates

### **Features**

- **Recursively scan** directories for Git repositories
- **Check** for:
	- Uncommitted changes
	- Unpushed commits
	- Unpulled changes
- **Flags** to show branches, paths or set a custom root folder

### **Install**

To install **git-check**, follow these steps:

1. **Clone the repository:**
   ```sh
   git https://github.com/giuseppepelusi/git-check.git
   ```

   or

   ```sh
   git git@github.com:giuseppepelusi/git-check.git
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

After installing, you can use the **git-check** command as follows:

- **Run a check starting from the current directory:**
    ```sh
    git-check
    ```

- **Specify a different directory to start scanning from (defaults to $HOME):**
    ```sh
    git-check -d <directory>
    ```

- **Show the current branch for each repository:**
    ```sh
    git-check -b
    ```

- **Show the full path instead of just the directory name:**
    ```sh
    git-check -p
    ```

- **Combine multiple flags as needed:**
    ```sh
    git-check -bpd ~/Projects
    ```

- **Show the list of available commands:**
    ```sh
    git-check help
    ```

### **Uninstall**

To uninstall **git-check**:

- **Run the uninstaller script:**
    ```sh
    make uninstall
    ```
