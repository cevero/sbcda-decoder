classdef PttA2
  % Objects from this class contais all parameters of a PTT-A2 baseband signal.
  properties (SetAccess=protected)
    userMsg;       % User Message
    % userMsgLength information coded in a integer between 1 and 8.
    % msgLenType is used to select the User Message Length between the
    % following allowed values
    % 56, 88, 120, 152, 184, 216, 248 and 280.
    msgLenType;
    psf;  % pulse shapping filter inpulse response
  end
  properties (Constant)
    fs              = 128e3;       % sample rate
    bitRate         = 400;         % data rate in bits per second
    angMod          = pi/3;        % modution angle in rad
    tCarrier        = 0.16;        % pure carrier period time length
    % sync bit pattern
    synchPattern    = [1 1 1 1 1 1 1 1 ...  
      1 1 1 1 1 1 1 0 ...
      0 0 1 0 1 1 1 1];
  end
  properties (Dependent)
    userMsgLength;                 % User Mensage Length in bits
    timeLength;                    % signal time length in seconds
  end
  
  methods
    
    function obj = PttA2(msgLenType)
      % Initialze User Message Length randomly if not provided
      if (nargin == 0)
        msgLenType = randi([1,8]);
      elseif ( msgLenType==0)
        display('Error: msgLenType invalid');
      end
      obj.msgLenType = msgLenType;
      
      % Initalize User Message randomly
      obj = obj.genRandUserMsg;
      
      
      % Generate trapezoidal pulse shapping filter
      % 'beta' is the transition period in multiple of PSK symbol period
      beta = 0.125;  % value between 0 and 1
      %beta = 0;

      pskRate = 2*obj.bitRate;
      lessBetaSmpl = round((1-beta)/2*(obj.fs/pskRate));
      plusBetaSmpl = round((1+beta)/2*(obj.fs/pskRate));
      
      obj.psf = zeros(1,2*plusBetaSmpl+1);
      idx = 0;
      for n=-plusBetaSmpl:plusBetaSmpl
        idx = idx+1;
        if(abs(n)<=lessBetaSmpl)
          obj.psf(idx)=1;
        else
          obj.psf(idx)=1-(abs(n)-lessBetaSmpl)/(plusBetaSmpl-lessBetaSmpl);
        end
      end
      
    end
    
    
    function value = get.userMsgLength(obj)
      value = 24+32*obj.msgLenType;
    end
    
    
    function value = get.timeLength(obj)
      value = obj.tCarrier + ...
        (length(obj.synchPattern)+obj.userMsgLength)*1/obj.bitRate + ...
        (length(obj.psf)-1)/obj.fs;
    end
    

    function value = digitalMsg(obj)
    % Return the Ptt-A2 Digital Message.
    % Overall bits entering the modulator, as defined in the spec.
      value = [obj.synchPattern obj.userMsg];
    end
    
    function obj = genRandUserMsg(obj)
      % Randomize User Message (userMsg)

      % convert mLenCodeList in a bit vector
      aux = dec2bin(obj.msgLenType-1, 3) == '1';
      
      % generate the parity bit
      parity = mod(sum(aux), 2);
      
      % append the parity bit to the bit vector
      mLenCodeBitVec = [aux parity];
      
      % generate random ID number and User Data
      randomDataBitVec = round(rand(1,obj.userMsgLength-4));
      
      % full user message generation
      obj.userMsg = [mLenCodeBitVec randomDataBitVec];
    end
    
    
    function pttSignal = signalGen(obj)
      % Generate the PTT Signal
      
      % Append synchPattern to userMsg to create the full tx bit vector
      bitVec =  [obj.synchPattern obj.userMsg];
      
      % apply biphase_L coding 1-->[+1 -1], 0-->[-1 +1];
      symbVec0 = 2*bitVec-1;
      symbVec1 = [symbVec0 ; -symbVec0];
      symblVec2 = reshape(symbVec1, 1, []);
      
      % add zeros for Pure Carrier period
      symblVec = [zeros(1, obj.tCarrier*2*obj.bitRate) symblVec2];
      
      % constelation mapping
      symbVec = exp(1j*obj.angMod*symblVec);
      
      % upsampling according to ratio between sample frequency and 2*BitRate.
      upSmplRate = obj.fs/(2*obj.bitRate);
      pttSignal = upsample(symbVec, upSmplRate);
      
      % apply pulse shapping filter
      % rectangular shape filter is used
      % rec = ones(1,upSmplRate);
      % pttSignal = filter(rec, 1, pttSignal);
      
      % trapezoidal shape filter
      pttSignal = conv(obj.psf, pttSignal);
    end
    
  end
end

