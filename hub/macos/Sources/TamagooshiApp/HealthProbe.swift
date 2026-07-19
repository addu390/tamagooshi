import Foundation

final class HealthProbe {
    private let url: URL
    private let onChange: (Bool) -> Void
    private var timer: Timer?
    private var healthy: Bool?

    init(baseURL: URL, onChange: @escaping (Bool) -> Void) {
        self.url = baseURL.appendingPathComponent("healthz")
        self.onChange = onChange
    }

    func start(interval: TimeInterval = 3) {
        stop()
        timer = Timer.scheduledTimer(withTimeInterval: interval, repeats: true) { [weak self] _ in
            self?.poll()
        }
        poll()
    }

    func stop() {
        timer?.invalidate()
        timer = nil
    }

    private func poll() {
        var request = URLRequest(url: url, timeoutInterval: 2)
        request.cachePolicy = .reloadIgnoringLocalCacheData
        URLSession.shared.dataTask(with: request) { [weak self] _, response, _ in
            let ok = (response as? HTTPURLResponse)?.statusCode == 200
            DispatchQueue.main.async { self?.report(ok) }
        }.resume()
    }

    private func report(_ ok: Bool) {
        guard ok != healthy else { return }
        healthy = ok
        onChange(ok)
    }
}
