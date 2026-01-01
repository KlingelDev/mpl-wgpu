// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Simple Rust example using mpl-wgpu

use mpl_wgpu::Figure;

fn main() -> anyhow::Result<()> {
    println!("mpl-wgpu Simple Plot Example (Rust)");

    // Create figure
    let mut fig = Figure::new();

    // Create data
    let x: Vec<f64> = (0..20).map(|i| i as f64).collect();
    let y: Vec<f64> = x.iter().map(|&x| x * x).collect();

    // Plot
    fig.plot(&x, &y)?;

    // Show
    fig.show()?;

    println!("Plot rendered successfully!");
    Ok(())
}
