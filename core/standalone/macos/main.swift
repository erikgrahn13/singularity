import SwiftUI
import WebKit
import SingularityController

@MainActor
class AppState: ObservableObject {
    // let audioEditor = createEditorInstance()
}

@main
struct MyApp : App {
    @StateObject private var appState = AppState()
    
    var body: some Scene {
        WindowGroup {
            WebViewRepresentable(url: URL(string: "http://localhost:5173")!)
                .environmentObject(appState)
        }
        .defaultSize(width: 100, height: 200)

    }
}

struct WebViewRepresentable: NSViewRepresentable {
    let url: URL
    
    func makeNSView(context: Context) -> WKWebView {
        let webView = WKWebView()
        webView.load(URLRequest(url: url))
        return webView
    }
    
    func updateNSView(_ nsView: WKWebView, context: Context) {}
}