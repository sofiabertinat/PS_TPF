from ctypes import sizeof
import numpy   as     np
from scipy import signal
import matplotlib.pyplot    as     plt
np.set_printoptions(precision=3, suppress=False)
import simpleaudio as sa
import matplotlib.patches

fs = 10000 
p1 = np.load("adc.npy")

N1 = len(p1)
print(N1)

t = np.linspace(0, 1, N1, endpoint=True)

nData1  = np.arange(0,N1,1) 
fData1  = nData1*(fs/N1)-fs/2
arrayp1 = np.array(p1)
fft1 = np.fft.fft(arrayp1)    
fft1 = np.fft.fftshift(fft1)
sp1 = np.abs(fft1/N1)**2 

plt.subplot(2,1,1)
plt.title('Signal audio original')
plt.xlabel('time') 
plt.ylabel('Amplitud') 
plt.plot(t, p1,linewidth=1,alpha=0.5)

plt.subplot(2,1,2)
plt.title('Espectro audio original, N='+str(N1))
plt.xlabel('Frecuency') 
plt.ylabel('Density of spectral power') 
plt.plot(fData1, sp1,linewidth=1,alpha=0.5)

plt.show()


for i in range(2):
    play_obj = sa.play_buffer(p1, 1, 2, 8000)
    play_obj.wait_done()
