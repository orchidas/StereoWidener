# StereoWidener
Plugin to do stereo widening with decorrelation.

Stereo widening widens the perception of the stereo image by boosting the surrounds and lowering the mid/side ratio. This plugin has controls for the stereo width in lower and higher frequencies, with a controllable cutoff frequency.
The filterbank consists of a lowpass and highpass in cascade, which can be energy preserving (Butterworth, default), or amplitude preserving (Linkwitz-Riley). Two options for decorrelation are available -- efficient convolution with velvet noise filters (default), or a cascade of allpass filters with randomised phases. A transient handling block ensures that transients are not smeared by the decorrelating filters.

<p align = "center">
<img width="300" alt="Screen Shot 2023-10-21 at 10 55 50 AM" src="https://github.com/orchidas/StereoWidener/assets/18227419/f12000bc-662f-4aa8-af66-901c1eebd225">
</p>

### For MacOS users
Copy the AU/VST3 build (in Builds/MacOSX/build/Debug) to /Library/Audio/Plugins/Components or /Library/Audio/Plugins/VST3. These are the .component and vst3 files respectively. The next time you restart your DAW the plugin should show up under the developer name **orchi**.

### Theory
Coming soon...
