classdef  SimParam
  % SimParam objects holds all simulation parameters
  
  properties
    fileName;        % name of the file that will store the simulation result
    comment;         % a sentece describing the simulation scenario
    reset;           % if reset=1 overwrite previous saved simulation
    n0DbFs;          % noise power density in dB ratio to ADC Clip (Saturation) Point Power
    fullScaleAmp;    % ADC full scale amplitude
    pttList;         % cell list containing PttA2 and PttA3Nz objects
    maxPwrDbN0;      % Maximum PTT power (realistic estimation)
    dynRangeDb;      % dB ratio between strongest and weakest PTT signal. 
    peakPwrDbN0;     % Worst case instantaneous power peak in dB ratio to N0.
    pwrDbN0List;     % PTTs power in dB ratio to No (noise power density)
    thetaDegList;    % PTTs initial phase in degree
    freqHzList;      % PTTs initial frequency in Hz
    hzPerSecList;    % PTTs Doppler Rate in Hz/s
    tSim;            % simulation time length in second
    timeList;        % PTTs initial time in seconds
    vectSignalGen;   % Generate signal for Vect. Signal Generator
    
    nDecoder=12;     % Number of PTT Decoder channel
    freqW=20;        % Decoder frequency word width
  end
  
  properties (Dependent)
    typeList;        % PTT signals type (A2-> 2, A3/Nz->3)
    userMsgLenList;  % List of User Message Lengths in bits
  end
  
  properties (SetAccess=protected)
    nPtt;            % number of PTT signal in simulation
    fs = 128e3;      % sample frequency
  end
  
  
  methods
    
    function value = get.typeList(obj)
      value = zeros(1, obj.nPtt);
      for iPcd =1:obj.nPtt
        if isa(obj.pttList{iPcd}, 'PttA2')
          value(iPcd) = 2;
        else
          value(iPcd) = 3;
        end
      end
    end

    function value = get.userMsgLenList(obj)
      value = zeros(1, obj.nPtt);
      for iPcd = 1:obj.nPtt
        value(iPcd) = obj.pttList{iPcd}.userMsgLength;
      end
    end
    
    function obj = SimParam(nPtt, tSim, typeList, usrMsgLenCode)
      % Create a SimParam object
      % SINTAX
      % obj = SimParam(nPtt, simTime, typeList, usrMsgLenCode)
      % nPtt: number of Ptt signals. If undefined, it assumes 10.
      % tSim: simulation time length in seconds.
      % typeList: a 1 x nPtt array that specifies each Ptt signal type as PTT-A2 
      % or PTT-A3 using the values of 2 and 3, respectively.
      % usrMsgLengCode: 1 x nPtt array that specifies each Ptt signal length
      % with an interger value from 0 to 8 for PTT-A3, and from 1 to 8 for
      % PTT-A2.
      % If tSim is not provided it will receive the default value of nPtt/5+1;
      % If typeList and usrMsgLenCode are not provided they will be randomized.       

      % set a default value for input arguments, if not provided. 
      if (nargin<1)
        nPtt = 10;
      end
      if (nargin<2)
         tSim = nPtt/5+1; 
      end
      if (nargin<3)
        % Set PTT types randomly: if argos 2 or 3.
        % typeList = randi([2,3],1,nPtt);
        % Set all PTT type to A2
        typeList = 2*ones(1,nPtt);
      end
      if (nargin<4)
        % Set User Message Length randomly
        usrMsgLenCode =  zeros(1, nPtt);
        for iPcd=1:nPtt
          if typeList(iPcd)==2
            usrMsgLenCode(iPcd) = randi([1,8]);
          else
            usrMsgLenCode(iPcd) = randi([0,8]);
          end
        end
      end
      obj.nPtt = nPtt;
      
      obj.fileName = '/save/temp_save.mat';
      obj.reset = 0;
      obj.comment = ['Simulation with ' num2str(obj.tSim) ' seconds, and '...
        num2str(obj.nPtt) ' PTTs randomly spread in time and frequency'];
  
      % Create PTT Objects
      obj.pttList = cell(1,obj.nPtt);
      for iPcd = 1:obj.nPtt
        if typeList(iPcd)== 2
          obj.pttList{iPcd} = PttA2(usrMsgLenCode(iPcd));
        else
          obj.pttList{iPcd} = PttA3Nz(usrMsgLenCode(iPcd));
        end
      end
      
      % Set Simulation time
      obj.tSim = tSim;
      
      % Random carrier power, phase and doppler
      obj.maxPwrDbN0 = 64;
      obj.dynRangeDb = 24;
      obj.pwrDbN0List = obj.maxPwrDbN0-obj.dynRangeDb*rand(1,obj.nPtt);
      obj.thetaDegList = rand(1,obj.nPtt)*360;
      obj.hzPerSecList = 240*rand(1,obj.nPtt)-120;
      
      % Randomize time and frequency
      % obj = obj.randTimeFreq();
      obj.timeList = sort(rand(1,nPtt)*(tSim-1));
      obj.freqHzList  = rand(1,nPtt)*60e3-30e3;
      
      % Set ADC and noise parameters
      obj.fullScaleAmp = 2^15-1;
      obj.n0DbFs = -118.5; % -118.5 is the value measured
      obj.peakPwrDbN0 = obj.maxPwrDbN0+20; % 86 dBm - (-174 dBm/Hz)
      
      obj.vectSignalGen = false;
    end
    
    
    function obj = randTimeFreq(obj)
      % Generates a random list of frequencies and startup times to all PTTs, 
      % without conflicts.
      % Requires predefined values for the properties: nPtt and pttList. 
      % SYNTAX
      % SimParam = randTimeFreq(SimParam)
      
      % Creates a random list of startup times in ascending order.
      % To avoid PTT signal truncation, all PTT signals start before
      % the last second of the simulation.
      obj.timeList = sort((obj.tSim-1)*rand(1,obj.nPtt));
      
      % alocate memory
      obj.freqHzList = zeros(1,obj.nPtt);
      
      % Sets randomly the first frequency.
      obj.freqHzList(1) = fix(60000*rand)-30000;
      
      for iPtt = 2:obj.nPtt
        % Generate frequencies without creating a time-frequency
        % conflict between signals.
        conflict = true;
        while conflict
          % Sets iPtt frequency randomly
          newFreq = fix(60000*rand)-30000;
          
          % Look for time conflict with previous signals
          timeConflictIdx = obj.timeList(iPtt) < obj.endTime(1:iPtt-1);
          
          % Look for one frequency conflict with signals in time conflict
          freqTimeConflict = find(...
            abs(obj.freqHzList(timeConflictIdx)-newFreq) <= 1600, 1);
          
          % Keep doing this until no time-frequency conflict exist
          conflict = ~isempty(freqTimeConflict);
        end
        obj.freqHzList(iPtt) = newFreq;
      end      
    end
    
    
    function time = endTime(obj, pttIdxList)
      % Return the time in seconds that the Pcd transmission ends
      time = zeros(1, length(pttIdxList));
      for i0=1:length(pttIdxList)
        iPcd = pttIdxList(i0);
        time(i0) = obj.timeList(iPcd) + obj.pttList{iPcd}.timeLength;
      end
    end 
    
    
    function obj = genRandUserMsg(obj)
      % Randomize the user message of all Ptt signals 
      % SYNTAX
      % SbcdSimParam = genRandUserMsg(SbcdSimParam)
      for iPtt=1:obj.nPtt
         obj.pttList{iPtt}=obj.pttList{iPtt}.genRandUserMsg;
      end
    end
    
    
    function [outputSignal, meanPwrDbm] = signalGen(obj)
      % Generate signal and compute signal meanPwr
      % outputSignal: 
      % meanPwrDbm: 
           
      % Number of simulation samples
      nSimSmpl = ceil(obj.tSim*obj.fs);
      
      % Adjust amplitude to normalize noise power density
      pttRMS = 10.^(obj.pwrDbN0List/20);
      if (obj.vectSignalGen) 
        % No digitaly generated noise 
        outputSignal = zeros(1,nSimSmpl)+1j*zeros(1,nSimSmpl);
      else
        noisePwr = obj.fs; 
        outputSignal = sqrt(noisePwr/2)*(randn(1,nSimSmpl)+1j*randn(1,nSimSmpl));
      end
      
      for iPtt = 1:obj.nPtt
        
        pttSignal = obj.pttList{iPtt}.signalGen();
        
        % Generate carrier signal and apply it to pttSignal
        % theta(nT) = theta0 + w0*(nT) + 1/2 * delta_w*(nT)^2;
        n = 0:length(pttSignal)-1;
        % convert initial fase to rad
        theta0 = obj.thetaDegList(iPtt)*180/pi; 
        % convert initial frequency to rad/smpl
        w0 = 2*pi*obj.freqHzList(iPtt)/obj.fs ; 
        % convert Dopper Rate from Hz/s to rad/smpl^2
        delta_w = 2*pi*obj.hzPerSecList(iPtt)/(obj.fs^2);
        carrierSignal = exp(1j*(theta0 + w0*n + 1/2*delta_w*n.^2));
        pttSignal = pttRMS(iPtt)*carrierSignal.*pttSignal;
        
        % Add this PTT signal to the total, inlcuiding signal delay.
        
        % Generate PTT signal sample index array
        initIdx = round(obj.timeList(iPtt)*obj.fs)+1;
        lastIdx = initIdx+length(pttSignal)-1;
        pttIdxArray = initIdx:lastIdx;
        % Add PTT signal to the whole signal
        outputSignal(pttIdxArray) = outputSignal(pttIdxArray) + pttSignal;
      end
      
      
      % compute signal mean power in dBm
      meanPwr = mean(abs(outputSignal).^2);
      meanPwrDbm = 10*log10(meanPwr) - 174;
        
      if (obj.vectSignalGen)
        % normalize signal amplitude to use all VSG dynamic range
        gain=1/max(abs(outputSignal));
        outputSignal = gain*outputSignal*obj.fullScaleAmp;

      else
        % adjust amplitude to according to ADC fullScaleAmp
        gain = 10^(obj.n0DbFs/20);
        outputSignal = gain*outputSignal;
      end
      % apply ADC quantization noise (or DAC quantization noise for
      % VecSigGen)
      % outputSignal = obj.fullScaleAmp*outputSignal;
      outputSignal = round(obj.fullScaleAmp*outputSignal);
    end 
    
  end
end

