% This function simulates the pttDetector using the folowing algorithm
% 
% ALGORITHM STEPS
%
% 1- Generate an input signal of 10 sec with 50 PTT-A2 signals with 920ms each
% and without conflic between them.
%
% 2- Break the input signal in windows of 1280 samples;
%
% 3- Set Decoders initial state and start the simulation.
%
% 4- Feed the detector with next input window and decoder states; 
%
% 5- In case of correct detection, update the decoder states and increment
% the nDetection; otherwise, increment nFalseDetection.
%
% 6- Change decoder state from busy to free if signal end time has passed.
%
% 7- Go to step 4 if simulation is not done;
%
% 8- Print nSignal, nDetection, nFalseDetection 


clear variables

stream = RandStream.getGlobalStream;
reset(stream);

%% STEP 1
% Generate an input signal of 10 sec with 100 PTT signals with .6 second
% each and without conflic between them.

% create a SimParam object

nPtt = 50;
tSim = 10;
maxPwrDbN0 = 64;
dynRangeDb = 24;
worstCNO = maxPwrDbN0 - dynRangeDb;
typeList = 2*ones(1,nPtt);
usrMsgLenCode = 8*ones(1,nPtt);

param = SimParam(nPtt, tSim, typeList, usrMsgLenCode);
param.maxPwrDbN0 = maxPwrDbN0;
param.pwrDbN0List = maxPwrDbN0-dynRangeDb*rand(1,param.nPtt);
param.freqHzList = rand(1,nPtt)*60e3-30e3;
param.hzPerSecList = rand(1,nPtt)*240-120;
param.timeList = sort(rand(1,nPtt)*(tSim-1));

% Generate the input signal
inputGain = 10^-((param.peakPwrDbN0+param.n0DbFs)/20);
inputSignal = round(inputGain*signalGen(param));


%% STEP 2
% Break the input signal in windows of 1280 samples

% compute number of windows
nWindow = ceil(length(inputSignal)/1280);
% adjust input signal length to a multiple of 1280 
nSmplToAdd = nWindow*1280 - length(inputSignal);
inputSignalExt = [inputSignal zeros(1, nSmplToAdd)];
% create array of windows
inputWindArray = reshape(inputSignalExt, 1280, nWindow).';


%% STEP 3
% Set simulation initial state

% create Ptt Detector system
%pttDetectorSys = pcdDetector2(param);
pttDetectorSys = pttDetector(param);
nDecoder = pttDetectorSys.nDecoder; % max num of concurrent decoding processes

% set decoder state
decoder(nDecoder) = struct;
for iDecoder=1:nDecoder
    decoder(iDecoder).busy = false;
    decoder(iDecoder).setupFreq = int32(0);
    decoder(iDecoder).setupAbs = 0;
    decoder(iDecoder).pcdIdx = 0;
    decoder(iDecoder).endTime = 0;
end

nDetection = 0;                   % number of correct detection
nLateDetec = 0;                   % number of late detection
nFalseDetection = 0;              % number of false detection
detectedIdxList = zeros(1, nPtt);  % list of index of detected PCDs
detectedTimeList = zeros(1, nPtt); % list of detected time.
detectedFreqList = zeros(1, nPtt); % list of detected frequency;
% Detector System input and output frequency information is given in a
% integer multiple of freqRes
freqRes = param.fs/2^pttDetectorSys.freqW;

for iWindow = 1:nWindow
    %% STEP 4
    % Feed the detector with next input window and decoder states
    
    currentTime = (iWindow-1)*10e-3;        % update current time
    inputWind = inputWindArray(iWindow, :);
    [pttDetectorSys, detectPos, detectAmp, detectValid] = ...
      pttDetectorSys.step(inputWind, decoder);
    
    for iDecod=1:nDecoder
        % if a signal was detected
        if detectValid(iDecod)
            dFreqHz = double(detectPos(iDecod))*freqRes;
            dAmp = detectAmp(iDecod);
            
            % check if this signal really exist in the input signal
            falseDetect = 1;
            for iPtt=1:nPtt
                sameFreq = abs(dFreqHz - param.freqHzList(iPtt)) < 65;
                % check if current time is inside this PTT transmission period, 
                % WARNING: If double detection is not used, detection instant 
                % can be up to 10ms lower than signal start time.
                sameTime = ...
                    (currentTime >= param.timeList(iPtt)) && ...
                    (currentTime <= param.endTime(iPtt));

                
                % In case of correct detection
                if (sameFreq && sameTime)
                    falseDetect = 0;
                    % increment number of detected signal
                    nDetection = nDetection + 1;
                    
                    % add this signal to a decoder
                    decoder(iDecod).busy = true;
                    decoder(iDecod).pcdIdx = iPtt;
                    decoder(iDecod).setupFreq = int32(param.freqHzList(iPtt)/freqRes);
                    decoder(iDecod).setupAbs = dAmp;
                    decoder(iDecod).endTime = param.endTime(iPtt);
                    
                    % save the index of this PCD;
                    detectedIdxList(nDetection) = iPtt;
                    detectedTimeList(nDetection) = currentTime;
                    detectedFreqList(nDetection) = dFreqHz;
                    detectedAmpList(nDetection) = dAmp;
                    
                    % if after pure carrier period counts as late detection
                    inTime = ...
                      (currentTime >= param.timeList(iPtt)) && ...
                      (currentTime <= param.timeList(iPtt)+160e-3);
                    
                    if (~inTime)
                      nLateDetec = nLateDetec + 1;
                    end
                    break
                end
            end
            % Increment nFalseDetection if no match was found.
            nFalseDetection = nFalseDetection + falseDetect;
        end
    end


    %% STEP 6
    % Change decoder state from busy to free if its decoding signal end time
    % have been reached.
    for iDecod=1:nDecoder
        if (decoder(iDecod).busy) && (currentTime > decoder(iDecod).endTime)
            decoder(iDecod).busy = false;
            decoder(iDecod).setupFreq = 0;
            decoder(iDecod).setupAbs = 0;
            decoder(iDecod).pcdIdx = 0;
            decoder(iDecod).endTime = 0;
        end
    end
    
    % Print a warning in case of all decoder slot are occupied
    allBusy = 1;
    for iDecod=1:nDecoder
        if ~decoder(iDecod).busy
            allBusy = 0;
        end
    end
    if allBusy
        display(['Warning: maximum number of detection reached at ' num2str(currentTime) 's'])
    end
    
end
    
    
%% STEP 7
% Display simulation result

nMissed = nPtt-nDetection;
display(nDetection);
display(nLateDetec);
display(nMissed);
display(nFalseDetection);


%load('main_home');
filename = [pwd '/output/pcdDetectorSimResult'];
titleRegion = 'A1:H1';
dataRegion = ['A2:H' num2str(nPtt+1)];
resumeTitleRegion = ['A' num2str(nPtt+2) ':D' num2str(nPtt+2)];
resumeDataRegion = ['A' num2str(nPtt+3) ':D' num2str(nPtt+3)];
display(['Simulation result in file ' filename]);

[~, timeSorted] = sort(detectedTimeList(1:nDetection));
detectedIdxList = detectedIdxList(timeSorted);
detectedTimeList = detectedTimeList(timeSorted);
detectedFreqList = detectedFreqList(timeSorted);
detectedAmpList = detectedAmpList(timeSorted);
detectedPwrList = 20*log10(detectedAmpList/param.fullScaleAmp) + param.peakPwrDbN0;

A={'detected', 'detectTime', 'startTime', 'endTime', 'freq', 'amp', 'freqErr', 'ampErr (dB)'}; 
csvwrite(filename,titleRegion);

A = zeros(nPtt, 7);
for iPtt=1:nPtt
    idx=find(detectedIdxList==iPtt, 1);
    if isempty(idx)
       A(iPtt, 1) = 0;
       A(iPtt, 2) = 0;
       A(iPtt, 7) = 0;
       A(iPtt, 8) = 0;
    else
       A(iPtt, 1) = 1;
       A(iPtt, 2) = detectedTimeList(idx);
       A(iPtt, 7) = detectedFreqList(idx) - param.freqHzList(iPtt);
       A(iPtt, 8) = detectedPwrList(idx) - param.pwrDbN0List(iPtt);
    end
    A(iPtt, 3) = param.timeList(iPtt);
    A(iPtt, 4) = param.endTime(iPtt);
    A(iPtt, 5) = param.freqHzList(iPtt);
    A(iPtt, 6) = param.pwrDbN0List(iPtt);
end
csvwrite(filename, dataRegion);
    
resumeTitle = {'nSignal', 'nDetected', 'nMissed' 'nFalseDetect'};
csvwrite(filename, resumeTitleRegion);
A = [nPtt, nDetection, nMissed, nFalseDetection];
csvwrite(filename, resumeDataRegion);

