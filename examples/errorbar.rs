use mpl_wgpu::{ErrorData, PlotBackend, PrimitiveRenderer, TextRenderer};
use glam::Vec4;

/// Example demonstrating error bar usage
fn main() {
    // Sample experimental data with measurement errors
    let x: Vec<f64> = (0..20).map(|i| i as f64).collect();
    
    // Simulated measurements
    let y: Vec<f64> = x.iter()
        .map(|&x| 5.0 + 0.3 * x + (x * 0.5).sin() * 2.0)
        .collect();
    
    // Symmetric errors (e.g., ±0.5 units)
    let y_err: Vec<f64> = vec![0.5; x.len()];
    
    // Asymmetric errors (different above and below)
    let y_err_low: Vec<f64> = vec![0.3; x.len()];
    let y_err_high: Vec<f64> = vec![0.8; x.len()];
    
    // X-axis measurement uncertainty
    let x_err: Vec<f64> = vec![0.2; x.len()];
    
    println!("Error Bar Examples");
    println!("==================");
    println!();
    println!("Example 1: Symmetric Y error bars");
    println!("Example 2: Both X and Y error bars");
    println!("Example 3: Asymmetric Y error bars");
    println!();
    println!("This demonstrates the three error bar methods:");
    println!("  - errorbar(x, y, y_err, color)");
    println!("  - errorbar_xy(x, y, x_err, y_err, color)");
    println!("  - errorbar_asymmetric(x, y, y_err_low, y_err_high, color)");
    
    // Note: This is a minimal example showing the API
    // In practice, you would integrate this with a windowing system
    // and wgpu context to render the plot
}
