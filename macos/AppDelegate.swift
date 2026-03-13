import Cocoa
import SingularityGraphics

class CanvasView: NSView {
    var graphics: SingularityGraphics?

    override func draw(_ dirtyRect: NSRect) {
        guard let ctx = NSGraphicsContext.current?.cgContext,
              let pixelData = graphics?.getPixels() else { return }

        let width    = Int(graphics!.getWidth())
        let height   = Int(graphics!.getHeight())
        let rowBytes = graphics!.getRowBytes()

        guard let provider = CGDataProvider(
            dataInfo: nil,
            data: pixelData,
            size: rowBytes * height,
            releaseData: { _, _, _ in }
        ) else { return }

        // Skia kN32 = BGRA_8888 premul → premultipliedFirst + byteOrder32Little
        let bitmapInfo = CGBitmapInfo(rawValue:
            CGImageAlphaInfo.premultipliedFirst.rawValue |
            CGBitmapInfo.byteOrder32Little.rawValue
        )

        guard let image = CGImage(
            width: width, height: height,
            bitsPerComponent: 8, bitsPerPixel: 32,
            bytesPerRow: rowBytes,
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: bitmapInfo,
            provider: provider,
            decode: nil, shouldInterpolate: false,
            intent: .defaultIntent
        ) else { return }

        ctx.draw(image, in: bounds)
    }

    // Mouse
    override func mouseDown(with event: NSEvent)      { print("left mouse clicked") }
    override func mouseUp(with event: NSEvent)        { /* left button released */ }
    override func mouseDragged(with event: NSEvent)   { /* left button held + moved */ }
    override func mouseMoved(with event: NSEvent)     { /* moved (needs acceptsFirstResponder) */ }
    override func rightMouseDown(with event: NSEvent) { /* right button */ }
    override func scrollWheel(with event: NSEvent)    { /* scroll / trackpad */ }
    

}

@main
class AppDelegate : NSObject, NSApplicationDelegate {

    var window: NSWindow!

    static func main() {
        let app = NSApplication.shared
        app.setActivationPolicy(.regular)
        let delegate = AppDelegate()
        app.delegate = delegate
        app.run()
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 480, height: 270),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.title = "My App"

        let canvasView = CanvasView(frame: NSRect(x: 0, y: 0, width: 480, height: 270))
        canvasView.graphics = SingularityGraphics()
        window.contentView = canvasView

        window.center()
        window.makeKeyAndOrderFront(nil)
        NSApp.activate()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }

    func applicationWillTerminate(_ notification: Notification) {
    }
}
