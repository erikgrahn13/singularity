import SingularityController
import StandaloneHelper
import SwiftUI
import WebKit

@MainActor
class AppState: ObservableObject {
    let controller = createControllerInstance()

    func initializeView(with window: UnsafeMutableRawPointer) {
        // Get raw pointer from unique_ptr and call C++ static method
        // Swift C++ interop: use __getUnsafe() or get() to access the raw pointer
        let controllerPtr = controller.__getUnsafe()
        StandaloneHelper.initializeView(controllerPtr, window)

    }
}

@main
struct MyApp: App {
    @StateObject private var appState = AppState()

    var body: some Scene {
        WindowGroup {
            WebViewRepresentable(appState: appState, url: URL(string: "http://localhost:5173")!)
        }
        .defaultSize(width: 800, height: 600)
    }
}

struct WebViewRepresentable: NSViewRepresentable {
    @ObservedObject var appState: AppState
    let url: URL

    func makeNSView(context: Context) -> NSView {
        // Create a container NSView (this is like your HWND on Windows)
        let containerView = NSView()

        // Get the raw pointer to the container view
        let viewPtr = UnsafeMutableRawPointer(Unmanaged.passUnretained(containerView).toOpaque())

        // Initialize - this should create the WebView INSIDE the container
        appState.initializeView(with: viewPtr)

        return containerView
    }

    func updateNSView(_ nsView: NSView, context: Context) {}
}
