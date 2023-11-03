# StereoWidener
Plugin to do stereo widening with decorrelation.

Stereo widening widens the perception of the stereo image by boosting the surrounds and lowering the mid/side ratio. This plugin has controls for the stereo width in lower and higher frequencies, with a controllable cutoff frequency.
The filterbank consists of a lowpass and highpass in cascade, which can be energy preserving (Butterworth, default), or amplitude preserving (Linkwitz-Riley). Two options for decorrelation are available -- efficient convolution with velvet noise filters (default), or a cascade of allpass filters with randomised phases.

<p align = "center">
<img width="345" alt="Screen Shot 2023-10-21 at 10 55 50 AM" src="https://github.com/orchidas/StereoWidener/assets/18227419/e55f4fcd-f91a-406a-92fb-c474f27cd56a">
</p>

### Theory
Coming soon...


<p align = "center"> 
<img width = "500" alt  = "Stereo widener basic" src = "https://github.com/orchidas/StereoWidener/assets/18227419/50444f92-c713-4f5d-9d86-9d67b4827ff1">
  <br></br>
  Stereo widening block diagram.
</p>
