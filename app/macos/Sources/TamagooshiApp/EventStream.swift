import Foundation

struct HubEvent {
    let type: String
    let data: [String: Any]
}

final class EventStream {
    private let url: URL
    private let onEvent: (HubEvent) -> Void
    private var task: Task<Void, Never>?

    init(baseURL: URL, onEvent: @escaping (HubEvent) -> Void) {
        self.url = baseURL.appendingPathComponent("api/events")
        self.onEvent = onEvent
    }

    func start() {
        stop()
        task = Task { [url, onEvent] in
            while !Task.isCancelled {
                do {
                    let (bytes, _) = try await URLSession.shared.bytes(from: url)
                    var type = ""
                    for try await line in bytes.lines {
                        if line.hasPrefix("event: ") {
                            type = String(line.dropFirst(7))
                        } else if line.hasPrefix("data: ") {
                            let payload = Data(line.dropFirst(6).utf8)
                            let json = try? JSONSerialization.jsonObject(with: payload)
                            let data = json as? [String: Any] ?? [:]
                            let event = HubEvent(type: type, data: data)
                            await MainActor.run { onEvent(event) }
                        }
                    }
                } catch {}
                try? await Task.sleep(for: .seconds(3))
            }
        }
    }

    func stop() {
        task?.cancel()
        task = nil
    }
}
