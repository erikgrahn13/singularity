import AppKit

@_expose(Cxx)
public func createChildNSView(parentHandle: UnsafeMutableRawPointer, width: Int32, height: Int32)
    -> UnsafeMutableRawPointer
{
    let parentView = Unmanaged<NSView>.fromOpaque(parentHandle).takeUnretainedValue()

    let frame = CGRect(x: 0, y: 0, width: CGFloat(width), height: CGFloat(height))
    let childView = NSView(frame: frame)
    childView.autoresizingMask = [.width, .height]

    parentView.addSubview(childView)

    return Unmanaged.passRetained(childView).toOpaque()
}
