import os
import sys
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

DOCS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class ExtensionlessHandler(SimpleHTTPRequestHandler):
    def translate_path(self, path):
        resolved = super().translate_path(path)
        if not os.path.exists(resolved) and os.path.isfile(resolved + ".html"):
            return resolved + ".html"
        return resolved


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    os.chdir(DOCS)
    server = ThreadingHTTPServer(("", port), ExtensionlessHandler)
    print(f"serving docs on http://localhost:{port} (extensionless, like GitHub Pages)")
    server.serve_forever()


if __name__ == "__main__":
    main()
