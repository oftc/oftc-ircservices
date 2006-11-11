#!/usr/bin/ruby

lang = ARGV[0]

File.open(lang, 'r') do |file|
  i = 1
  file.readline
  file.each_line do |line|
    if line[0,1] != "\t" then
      puts "#define %s %d" % [line.chomp.rstrip, i]
      i += 1
    end
  end
end
