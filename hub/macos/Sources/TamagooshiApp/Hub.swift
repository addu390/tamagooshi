import Foundation

enum HubState: Equatable {
    case stopped
    case starting
    case running
    case failed(String)
}

protocol Hub: AnyObject {
    var baseURL: URL { get }
    var managesProcess: Bool { get }
    func start()
    func stop()
}

final class ExternalHub: Hub {
    let baseURL: URL
    let managesProcess = false

    init(baseURL: URL) {
        self.baseURL = baseURL
    }

    func start() {}
    func stop() {}
}

final class SubprocessHub: Hub {
    let baseURL: URL
    let managesProcess = true

    private let executable: URL
    private let port: Int
    private var process: Process?
    private var desired = false

    init(executable: URL, port: Int) {
        self.executable = executable
        self.port = port
        self.baseURL = URL(string: "http://127.0.0.1:\(port)")!
    }

    func start() {
        desired = true
        launch()
    }

    func stop() {
        desired = false
        process?.terminate()
        process = nil
    }

    private func launch() {
        guard process == nil, desired else { return }
        let proc = Process()
        proc.executableURL = executable
        var env = ProcessInfo.processInfo.environment
        env["TAMA_HUB_HOST"] = "127.0.0.1"
        env["TAMA_HUB_PORT"] = String(port)
        env["TAMA_PARENT_WATCH"] = "1"
        proc.environment = env
        proc.terminationHandler = { [weak self] _ in
            DispatchQueue.main.async {
                guard let self else { return }
                self.process = nil
                if self.desired {
                    DispatchQueue.main.asyncAfter(deadline: .now() + 1) { self.launch() }
                }
            }
        }
        do {
            try proc.run()
            process = proc
        } catch {
            NSLog("hubd failed to launch: \(error.localizedDescription)")
        }
    }
}

enum HubLocator {
    static func make() -> Hub {
        let env = ProcessInfo.processInfo.environment
        if let url = env["TAMA_HUB_URL"].flatMap(URL.init(string:)) {
            return ExternalHub(baseURL: url)
        }
        let port = env["TAMA_HUB_PORT"].flatMap(Int.init) ?? 8737
        if let override = env["TAMA_HUBD"] {
            return SubprocessHub(executable: URL(fileURLWithPath: override), port: port)
        }
        if let bundled = Bundle.main.url(forResource: "hubd", withExtension: nil,
                                         subdirectory: "hubd") {
            return SubprocessHub(executable: bundled, port: port)
        }
        return ExternalHub(baseURL: URL(string: "http://127.0.0.1:\(port)")!)
    }
}
