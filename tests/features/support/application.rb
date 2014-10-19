require_relative 'exceptions'
require 'open3'
require 'elif'

class App
    def self.start
        btcd_file = Pathname.new(File.join($btcd_binary_folder,'BitcoinDarkd')).expand_path
        raise ApplicationMissingException,"Could not find BitcoinDarkd in path #{btcd_file.to_s}" unless btcd_file.exist?
        puts "Starting BitcoinDarkd"
        File.open($log_file, 'w') {|file| file.truncate(0) } if $log_file.exist? # Clean log file
        cwd = Dir.pwd
        Dir.chdir(btcd_file.dirname)
        job = fork do
            exec "nohup #{btcd_file.to_s}"
        end
        _, status = Process.waitpid2(job)
        Dir.chdir(cwd)
        sleep 5
        if `ps aux | grep -i BitcoinDarkd | grep -v grep` != ""
            # Wait for ready signal in nohup.out file to show 'back from start'
            App.searchLog('back from start', 50)
            sleep 5 # Extra time to connect to some peers
        end
        puts "Completely finished starting BitcoinDarkd"
    end

    def self.searchLog(text, timeout = 10)
        old_timeout = timeout
        while timeout > 0
            Elif.open($log_file).each_line do |l|
                return true if l.include?(text)
            end
            sleep 1 
            timeout -= 1
        end
        raise LogTimeoutException, "Spent #{old_timeout} seconds waiting for text \"#{text}\" to appear in #{$log_file}"
    end
end
