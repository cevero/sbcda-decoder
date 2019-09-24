classdef pttDetector
    % pttDetector
    % Receive as input a sequence of 2048 input samples
    
    properties (Constant)
        windowLength  = 1280;   % Window length in samples
        dftLength     = 2048;
        dftFreqW      = 11;     % log2(dftLength)
        sampleRate    = 128e3;  % Sample rate in Hz
        bw0Hz         = 1600;   % signal bandwidth in Hz
        bw1Hz         = 2400;   % signal bandwidth in Hz
        bw2Hz         = 3200;   % signal bandwidth in Hz
        worstCNO      = 40;     % Minimum detectable Carry to N0 dB ratio (aprox 12.7 Eb/N0)  
    end
    
    properties
        nDecoder;      % Number of decoders
        window;        % DFT window
        freqW;         % Frequency resolution in number of bits
        threshold;     % Detection threshold in Amplitude
        bw0;           % Signal bandwidth in multiple of sampleRate/dftLength
        bw1;           % Signal bandwidth upto 5th harmonic
        bw2;           % Signal bandwidth upto 7th harmonic
        prevPass;
    end
    
    methods
        function obj = pttDetector(param)
            if (nargin == 0)
                % Use default configuration
                param = SimParam();
            end
            obj.nDecoder = 12;
            obj.freqW = 20;
            % Detection threshold in dB ratio to ADC full scale power
            thHoldDbFs = obj.worstCNO-param.peakPwrDbN0;
            % Detection threshold amplitude.
            obj.threshold = int16(round(10^(thHoldDbFs/20)*param.fullScaleAmp));
            obj.bw0 = int16(floor(obj.bw0Hz*2^obj.dftFreqW/obj.sampleRate));
            obj.bw1 = int16(floor(obj.bw1Hz*2^obj.dftFreqW/obj.sampleRate));
            obj.bw2 = int16(floor(obj.bw2Hz*2^obj.dftFreqW/obj.sampleRate));
            obj.prevPass = false(1,obj.dftLength);
            % generate the window with the pi rad freqshift
            % window0 = 2*hann(2048, 'periodic')';
            % obj.window = ((-1).^(0:2047)).*window0;
            % window = (-1).^(0:1279);
            w = hamming(1280,'periodic')';
            w = 1/mean(w)*w;
            obj.window = ((-1).^(0:length(w)-1)).*w;
            
            % scalloping loss
            n = 0:length(w)-1;
            M = obj.dftLength;
            sclpLoss = abs(sum(w.*exp(1j*pi*n/M))/sum(w));
            sclpLossDb = 20*log10(sclpLoss);
        end % pttDetector
        
        
        function [obj, detectFreq, detectAmp, activeList] = step(obj, inputSeq, pttDecod)
            % Return a list of positions and absolute value of detected 
            % peaks in the power spectrum of the input signal. Peaks in
            % frequency close to busy PCD Decoders are not included.
            %
            % [detectFreq, powerList] = step(obj, windowIn, pttDecod)
            % obj: pttDetector system object
            % windowIn: input signal with 1280 samples
            % pttDecod: array of structure with information about the PCD Decoders
            %    pttDecod(id).busy: indicates if decoder is active;
            %    pttDecod(id).setupAbs:  amp of signal being decoded;
            %    pttDecod(id).setupFreq : frequency of signal being decoded in
            %    multiples of sampleRate/2^freqW
            % activeList: indicates an active pttDecoder slot
            % detectFreq: frequency of detected peaks in multiples of sampleRate/2^freqRes.
            % detectAmp: amplitude of detected peaks
            
            precDiff = obj.freqW-obj.dftFreqW;
            
            % Creates the output lists
            activeList = false(1,obj.nDecoder);
            detectFreq = int32(zeros(1,obj.nDecoder));
            detectAmp = zeros(1,obj.nDecoder);
            
            % Compute spectrum
            fftAmp = round(abs(fft(inputSeq.*obj.window,2048)/1280));
            
            % Compute mask
            mask = genMask(obj, pttDecod);
            
            % Intersting plot
%             figure(1)
%             %normFactor=10^(param.peakPwrDbN0/20)/param.fullScaleAmp;
%             normFactor = 1/2.0675;
%             x = ((0:obj.dftLength-1)-(obj.dftLength/2))*128e3/obj.dftLength;
%             y0 = 20*log10(fftAmp*normFactor);
%             y1 = 20*log10(double(mask*normFactor));
%             plot(x, y0, x, y1);
            
            % check for first pass
            % pass is a vector of boollean
            pass = fftAmp > mask;
            
            % check for double pass
            dPass = pass & obj.prevPass;
            obj.prevPass = pass;
            dpIdxList = find(dPass);
            % [obj, dpIdxList] = dualPass(obj, pass);
  
            % Compute number of free decoder
            nFreeDecoder = obj.nDecoder;
            for iDecod=1:obj.nDecoder
              nFreeDecoder = nFreeDecoder - pttDecod(iDecod).busy;
            end
            if nFreeDecoder==0
              return;
            end
                                    
            % Find peaks in double pass set
            iDecoder = 0;  
            while ~isempty(dpIdxList)
                % Gets the maximum peak in dual pass set
                [maxAmp, maxIdx] = max(fftAmp(dpIdxList));
                idx = dpIdxList(maxIdx); % get the DFT index of this peak
                
                % MaxPosition with fractional value
                % 1.13 if 1280-point retangular window
                % 2.586 if 1280-point hamming window
                fracIdx = 2.586*(fftAmp(idx+1)-fftAmp(idx-1))/sum(fftAmp(idx-1:idx+1));
                fltIdx = idx + fracIdx;
                % convert to multiple of smplRate/2^freqW
                freq = int32((fltIdx-obj.dftLength/2-1)*2^precDiff);
                
                % Peak amplitude estimation improved by Quadratic Interpolated 
                % FFT (QIFFT)
                maxAmp = uint16(...
                  maxAmp-1/4*(fftAmp(idx+1)-fftAmp(idx-1))*fracIdx);
                
                % Decrease the number of free decoders
                nFreeDecoder = nFreeDecoder - 1;
                
                % Find one unoccupied ptt decoder
                while iDecoder<obj.nDecoder
                    iDecoder = iDecoder + 1;
                    if ~pttDecod(iDecoder).busy
                        activeList(iDecoder) = true;
                        detectFreq(iDecoder) = freq;
                        detectAmp(iDecoder) = maxAmp;
                        nFreeDecoder = nFreeDecoder - 1;
                        break;
                    end
                end
                
                if nFreeDecoder==0
                  return;
                end
                
                % Remove power spectrum points close to the new peak
                dist = abs(dpIdxList-idx);
                ampl = fftAmp(dpIdxList);
                conflict = ...
                  (dist<obj.bw0) | ...
                  (dist<obj.bw1 & ampl<maxAmp/6) | ...
                  (dist<obj.bw2 & ampl<3*maxAmp/32);
                dpIdxList = dpIdxList(~conflict);
            end

        end % step
        
        
        function mask = genMask(obj, decod)
          precDiff = obj.freqW-obj.dftFreqW;
          
          mask = obj.threshold*int16(ones(1,obj.dftLength));
          for iDecod=1:obj.nDecoder
            if (decod(iDecod).busy)
              decodFreq = int16(bitshift(decod(iDecod).setupFreq,-precDiff));
              dftIdx = decodFreq+obj.dftLength/2+1;
              
              i0a = dftIdx-obj.bw2:1:dftIdx-obj.bw1-1;
              i0b = dftIdx+obj.bw1+1:1:dftIdx+obj.bw2;
              i0 = [i0a i0b];
              
              i1a = dftIdx-obj.bw1:1:dftIdx-obj.bw0-1;
              i1b = dftIdx+obj.bw0+1:1:dftIdx+obj.bw1;
              i1 = [i1a i1b];
              
              i2 = dftIdx-obj.bw0:dftIdx+obj.bw0;
              
              mask(i0) = max(mask(i0), 3*decod(iDecod).setupAbs/32);
              mask(i1) = max(mask(i1), decod(iDecod).setupAbs/6);
              mask(i2) = max(mask(i2), decod(iDecod).setupAbs*2);
            end
          end
        end        
        
        
        function [nDPass, dpIdxList] = dualPass(obj, X, M)
          
          dpIdxList = zeros(1,512);
          passIdxList = zeros(1,512);
          nPass = 0;
          nDPass = 0;
          i1 = 0;
          for i0=0:nDft-1
            % check first pass
            if X(i0) > M(i0)
              nPass = nPass + 1;
              passIdxList(nPass) = i0;
              
              % check for double pass
              while (i1<obj.nPrevPass && obj.prevPassIdxList(i1)<=i0)
                if (prevPassIdxList(i1)==i0)
                  nDPass = nDPass + 1;
                  dpIdxList(nDpass) = i0;
                  break
                end
                i1 = i1 + 1;
              end
            end
          end
          
          obj.nPrevPass = nPass;
          obj.prevPassIdxList = passIdxList;
          
        end

    end % Methods      
end
