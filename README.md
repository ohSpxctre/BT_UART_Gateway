

## 📚 Generate Documentation with Doxygen + Graphviz

This project includes a preconfigured `Doxyfile` for generating HTML documentation — including automatic class diagrams — from the source code.
The documentation includes:
   - 📄 Function/class/module descriptions
   - 🔍 Cross-referenced source code
   - 📊 UML-style class diagrams (if Graphviz is installed)

### 🛠️ How to Generate Documentation

Once you're set up, you have two options to generate documentation:

---

## 🐳 Option 1: Run in Docker (Dev Container)

If you're using the provided Dev Container (`.devcontainer/` folder):

1. Make sure `generate_docs.sh` is in the root of the project (outside `.devcontainer`).
2. The container setup is preconfigured to copy and install the script automatically via `postCreateCommand`.
3. All necessary tools — **Doxygen** and **Graphviz** — are already installed in the container image.
4. After the container builds and opens in VS Code, simply run:

   ```bash
   generate_docs.sh
   ```

📂 The output will appear in `docs/html/index.html` inside the container workspace.

✅ No extra installation is needed. This is the recommended method.

---

## 🧩 Option 2: Run Doxygen Locally (Host Machine)

Before generating documentation locally, you need to install the following tools:

| Tool      | Purpose                  | Download Link                          |
|-----------|--------------------------|----------------------------------------|
| **Doxygen**   | Documentation generator  | https://www.doxygen.nl/download.html  |
| **Graphviz**  | Class diagrams support   | https://graphviz.org/download/         |

> 💡 On Windows, make sure to check the **"Add to PATH"** option during installation.

To verify installation, run:

```bash
doxygen --version
dot -V
```

Then follow these steps:

1. Open a terminal or command prompt in the **root of the project**.
2. Run the following command:

   ```bash
   doxygen Doxyfile
   ```

3. If successful, the documentation will be generated in the folder specified by the `OUTPUT_DIRECTORY` in the `Doxyfile`.

   By default, this is:

   ```
   docs/html/index.html
   ```

4. Open `index.html` in your browser to view the generated documentation

---

