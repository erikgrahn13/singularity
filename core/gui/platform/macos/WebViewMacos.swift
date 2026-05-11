import WebKit

@_expose(Cxx)
public class WebViewMacos {
    private var webView: WKWebView?

    public init(windowHandle: UnsafeMutableRawPointer, cppAdapter: UnsafeMutableRawPointer) {
        let parentView = Unmanaged<NSView>.fromOpaque(windowHandle).takeUnretainedValue()

        let wkWebView = WKWebView(frame: parentView.bounds)
        wkWebView.autoresizingMask = [.width, .height]

        parentView.addSubview(wkWebView)

        self.webView = wkWebView
    }

    public func navigate(to url: String) {
        guard let url = URL(string: url) else { return }
        webView?.load(URLRequest(url: url))
    }
}

@_expose(Cxx)
public func createWebViewMacos(
    windowHandle: UnsafeMutableRawPointer, cppAdapter: UnsafeMutableRawPointer
) -> WebViewMacos {
    return WebViewMacos(windowHandle: windowHandle, cppAdapter: cppAdapter)
}
