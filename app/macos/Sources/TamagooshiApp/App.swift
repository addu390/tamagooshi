import AppKit

final class AppDelegate: NSObject, NSApplicationDelegate {
    private let hub = HubLocator.make()
    private let notifier = Notifier()
    private var statusItem: StatusItemController!
    private var dashboard: DashboardWindow!
    private var probe: HealthProbe!
    private var events: EventStream!
    private var activeAlerts = Set<String>()

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.accessory)
        notifier.requestPermission()

        statusItem = StatusItemController(managesProcess: hub.managesProcess)
        dashboard = DashboardWindow(baseURL: hub.baseURL)

        statusItem.onOpenDashboard = { [weak self] in self?.dashboard.show() }
        statusItem.onToggleHub = { [weak self] in self?.toggleHub() }

        probe = HealthProbe(baseURL: hub.baseURL) { [weak self] healthy in
            guard let self else { return }
            self.statusItem.setHealthy(healthy)
            self.statusItem.updateIcon(alerting: !self.activeAlerts.isEmpty)
        }
        events = EventStream(baseURL: hub.baseURL) { [weak self] event in
            self?.handle(event)
        }

        hub.start()
        probe.start()
        events.start()
    }

    func applicationWillTerminate(_ notification: Notification) {
        events.stop()
        probe.stop()
        hub.stop()
    }

    private func toggleHub() {
        if statusItem.hubRunning {
            hub.stop()
        } else {
            hub.start()
        }
    }

    private func handle(_ event: HubEvent) {
        switch event.type {
        case "mood":
            statusItem.setMood(event.data["mood"] as? String ?? "—")
        case "alert.raised":
            let id = event.data["id"] as? String ?? ""
            activeAlerts.insert(id)
            statusItem.updateIcon(alerting: true)
            notifier.post(title: event.data["title"] as? String ?? "Alert",
                          body: event.data["severity"] as? String ?? "")
        case "alert.cleared":
            activeAlerts.remove(event.data["id"] as? String ?? "")
            statusItem.updateIcon(alerting: !activeAlerts.isEmpty)
        case "device":
            notifier.post(title: "Device connected",
                          body: event.data["device_id"] as? String ?? "")
        default:
            break
        }
    }
}