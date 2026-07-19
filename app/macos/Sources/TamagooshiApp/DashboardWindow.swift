import AppKit
import WebKit

final class DashboardWindow: NSObject, NSWindowDelegate {
    private let url: URL
    private var window: NSWindow?

    init(baseURL: URL) {
        self.url = baseURL.appendingPathComponent("ui/")
        super.init()
    }

    func show() {
        if window == nil {
            let webView = WKWebView(frame: .zero)
            webView.load(URLRequest(url: url))

            let win = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 820, height: 640),
                               styleMask: [.titled, .closable, .resizable, .miniaturizable],
                               backing: .buffered, defer: false)
            win.title = "Tamagooshi"
            win.contentView = webView
            win.center()
            win.isReleasedWhenClosed = false
            win.delegate = self
            window = win
        }
        NSApp.activate(ignoringOtherApps: true)
        window?.makeKeyAndOrderFront(nil)
    }

    func windowWillClose(_ notification: Notification) {
        window = nil
    }
}
