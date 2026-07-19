// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "TamagooshiApp",
    platforms: [.macOS(.v13)],
    targets: [
        .executableTarget(name: "TamagooshiApp", path: "Sources/TamagooshiApp")
    ]
)
