


## Generate Documentation with Doxygen + Graphviz

This project includes a preconfigured `Doxyfile` for generating HTML documentation — including automatic class diagrams — from the source code.

### Prerequisites

Make sure the following tools are installed:

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

### 🛠️ How to Generate Documentation

Once the tools above are installed:

1. Open a terminal or command prompt in the **root of the project**.
2. Run the following command to generate the documentation:

   ```bash
   doxygen Doxyfile
   ```

3. If successful, the documentation will be generated in the folder specified by the `OUTPUT_DIRECTORY` in the `Doxyfile`.

   By default, this is:

   ```
   docs/html/index.html
   ```

4. Open `index.html` in your browser to view the generated documentation, including:
   - 📄 Function/class/module descriptions
   - 🔍 Cross-referenced source code
   - 📊 UML-style class diagrams (if Graphviz is installed)
