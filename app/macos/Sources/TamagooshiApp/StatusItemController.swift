import AppKit
import ServiceManagement

final class StatusItemController {
    private let item: NSStatusItem
    private let statusLine = NSMenuItem(title: "Starting…", action: nil, keyEquivalent: "")
    private let moodLine = NSMenuItem(title: "mood: —", action: nil, keyEquivalent: "")
    private let toggleItem = NSMenuItem(title: "Stop Hub",
                                        action: #selector(StatusItemController.toggleHub),
                                        keyEquivalent: "")
    private let loginItem = NSMenuItem(title: "Launch at Login",
                                       action: #selector(StatusItemController.toggleLogin),
                                       keyEquivalent: "")

    var onOpenDashboard: (() -> Void)?
    var onToggleHub: (() -> Void)?

    private(set) var hubRunning = false

    init(managesProcess: Bool) {
        item = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        item.button?.image = NSImage(systemSymbolName: "pawprint",
                                     accessibilityDescription: "Tamagooshi")

        let menu = NSMenu()
        statusLine.isEnabled = false
        moodLine.isEnabled = false
        menu.addItem(statusLine)
        menu.addItem(moodLine)
        menu.addItem(.separator())

        let open = NSMenuItem(title: "Open Dashboard", action: #selector(openDashboard), keyEquivalent: "d")
        open.target = self
        menu.addItem(open)

        toggleItem.target = self
        toggleItem.isHidden = !managesProcess
        menu.addItem(toggleItem)

        loginItem.target = self
        loginItem.state = SMAppService.mainApp.status == .enabled ? .on : .off
        menu.addItem(loginItem)

        menu.addItem(.separator())
        let quit = NSMenuItem(title: "Quit Tamagooshi", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
        menu.addItem(quit)

        item.menu = menu
    }

    func setHealthy(_ healthy: Bool) {
        hubRunning = healthy
        statusLine.title = healthy ? "Hub running" : "Hub unreachable"
        toggleItem.title = healthy ? "Stop Hub" : "Start Hub"
        updateIcon(alerting: false)
    }

    func setMood(_ mood: String) {
        moodLine.title = "mood: \(mood)"
    }

    func updateIcon(alerting: Bool) {
        let symbol = !hubRunning ? "pawprint.slash" : (alerting ? "exclamationmark.triangle" : "pawprint")
        item.button?.image = NSImage(systemSymbolName: symbol,
                                     accessibilityDescription: "Tamagooshi")
    }

    @objc private func openDashboard() { onOpenDashboard?() }

    @objc private func toggleHub() { onToggleHub?() }

    @objc private func toggleLogin() {
        let service = SMAppService.mainApp
        do {
            if service.status == .enabled {
                try service.unregister()
            } else {
                try service.register()
            }
        } catch {
            NSLog("launch at login toggle failed: \(error.localizedDescription)")
        }
        loginItem.state = service.status == .enabled ? .on : .off
    }
}
