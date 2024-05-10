import numpy as np
from collections import namedtuple
from typing import Optional, List
import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from scipy import interpolate
from scipy.signal import sosfreqz
from utils import db, hertz_to_erbscale, erbscale_to_hertz

FreqTicks = namedtuple("FreqTicks", ["ticks", "labels"])
AudFreqTicks = FreqTicks(ticks=[20, 50, 100, 250, 500, 1e3, 2e3, 4e3, 8e3, 16e3],
                         labels=[20, 50, 100, 250, 500, "1k", "2k", "4k", "8k", "16k"])


def erbspace(fmin: float, fmax: float, n: int) -> np.ndarray:
    """Generate a sequence of frequencies spaced evenly on the ERB-scale.

    Args:
        fmin (float): Minimum frequency (in Hertz)
        fmax (float): Maximum frequency (in Hertz)
        n (int): Number of points in the sequence.

    Returns:
        np.ndarray: The ERB-spaced sequence.
    """
    ymin, ymax = hertz_to_erbscale(fmin), hertz_to_erbscale(fmax)
    y = erbscale_to_hertz(np.linspace(ymin, ymax, n))
    y[0] = fmin
    y[-1] = fmax
    return y


def semiaudplot(
    f: np.ndarray,
    y: np.ndarray,
    *args,
    n: int = 1024,
    marker: Optional[str] = None,
    ax: Optional[Axes] = None,
    interp: bool = False,
    **pyplot_kwargs,
) -> List[plt.Line2D]:
    """Plot a sequence with one axis spaced using an auditory (ERB) frequency scale.

    Args:
        f (np.ndarray): Array of frequencies (in Hertz).
        y (np.ndarray): Data to be plotted (same size as f)
        n (int): Number of points to use for interpolation on auditory frequency scale, by default 1024
        marker (str): The marker style to use, by default None
        ax (Axes): Optionally provide the axes on which to plot, by default None
        interp (bool): If true, spline interpolation is performed for the plotted line, by default False.
        pyplt_kwargs (**kwargs): Additional keyword arguments are passed to the
            matplotlib.pyplot plotting function.

    Returns:
        List[plt.Line2D]: List of plotted lines.

    Raises:
        ValueError: y can only be up to two-dimensional
    """
    if ax is None:
        ax = plt.gca()
    f, y = np.asarray(f), np.asarray(y)
    if np.ndim(y) == 1:
        y = y[np.newaxis]
    elif np.ndim(y) != 2:
        raise ValueError("Only 1- or 2-dimensional arrays can be plotted")
    elif y.shape[1] != f.shape[0] and y.shape[0] == f.shape[0]:
        y = y.T

    fn = erbspace(np.min(f), np.max(f), n)

    plots = []
    kwargs_c = pyplot_kwargs.copy()
    for yi in y:
        if interp:
            tck = interpolate.splrep(f, yi)
            yy = interpolate.splev(fn, tck)
            plots.extend(ax.plot(fn, yy, *args, **pyplot_kwargs))
            if marker:
                kwargs_c["color"] = plots[-1].get_c()
                plots.extend(ax.plot(f, yi, marker=marker, linestyle='', *args, **kwargs_c, label=None))
        else:
            if marker:
                pyplot_kwargs['marker'] = marker
            plots.extend(ax.plot(f, yi, *args, **pyplot_kwargs))

    ax.set_xscale("function", functions=(hertz_to_erbscale, erbscale_to_hertz))
    ax.set_xticks(AudFreqTicks.ticks, AudFreqTicks.labels)
    ax.set_xlabel("Frequency (Hz)")
    return plots

def plot_icc_matrix(icc_matrix: np.ndarray,
                    freqs: np.ndarray,
                    num_channels: int,
                    chan_names: List[str],
                    title: str = 'ICC',
                    ax: Optional = None,
                    ear: Optional[str] = None,
                    room_name: Optional[str] = None,
                    ylimits: np.array = np.array([0, 1.05])):
    """Plots interchannel correlation matrix"""

    if ax is None:
        fig, ax = plt.subplots(num_channels, num_channels, figsize=[8, 8])
        if room_name is not None and ear is not None:
            fig.suptitle(f'{title} for {ear} ear in {room_name}')
        else:
            fig.suptitle(f'{title}')

    for i in range(num_channels):
        for j in range(num_channels):
            semiaudplot(freqs, icc_matrix[:, i, j], ax=ax[i, j])
            # Hide X and Y axes label marks and ticks
            if i < num_channels - 1 or j != num_channels // 2:
                ax[i, j].xaxis.set_tick_params(labelbottom=False)
                ax[i, j].set_xticks([])
                ax[i, j].set_xlabel('')

            if i == 0:
                ax[i, j].set_title(f'{chan_names[j]}')

            if j > 0:
                ax[i, j].yaxis.set_tick_params(labelleft=False)
                ax[i, j].set_yticks([])
                ax[i, j].set_ylabel('')

            if j == num_channels - 1:
                ax[i, j].set_title(f'{chan_names[i]}', loc='right')

            ax[i, j].set_ylim(ylimits)

    return ax


def plot_filt_response(sos, worN=1024):
    w, h = sosfreqz(sos, worN=worN)
    plt.subplot(2,1,1)
    plt.plot(w/np.pi, db(h))
    plt.ylim(-75, 5)
    plt.grid(True)
    plt.yticks([0, -20, -40, -60])
    plt.ylabel('Gain [dB]')
    plt.title('Frequency Response')
    plt.subplot(2,1,2)
    plt.plot(w/np.pi, np.angle(h))
    plt.grid(True)
    plt.yticks([-np.pi, -0.5*np.pi, 0, 0.5*np.pi, np.pi],
               [r'$-\pi$', r'$-\pi/2$', '0', r'$\pi/2$', r'$\pi$'])
    plt.ylabel('Phase [rad]')
    plt.xlabel('Normalized frequency (1.0 = Nyquist)')
