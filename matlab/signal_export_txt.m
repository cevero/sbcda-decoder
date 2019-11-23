% This script generates the text file used for testing the EDC Decoder Unit

% Create SimParm object
nPtt = 2;               % two PTT signals
tSim = 1.5;    % in seconds (this parameter will be overwrite)
typeList = [2 2];       % all signals are PTT-A2
userMsgLenCode = [1 1]; % both signals has minimal message length
param = SimParam(nPtt, tSim, typeList, userMsgLenCode);
param.timeList = [0 .04];
param.tSim = .5;

% Generate the signal
signal = param.signalGen();
% cut initial 120ms and final 140ms
% firstIdx = .120*param.fs+1;
% lastIdx = length(signal)-(.140*param.fs);
% signal = signal(firstIdx:lastIdx);

Fs = 128e3;
NFFT = 2048;
f_vec = [-floor(NFFT/2) : ceil(NFFT/2)-1] * Fs/NFFT;
spectrogram(signal,hamming(128),64,f_vec,Fs,'yaxis');

% Convert the signal to a text file, where 
% every line correspond to a complex sample
% with the real value followed by the imaginary value
fid=fopen('edc_m2s_tb.txt','w');
signal0 = [real(signal); imag(signal)];
fprintf(fid, '%d %d \n', signal0);
fclose(fid);


