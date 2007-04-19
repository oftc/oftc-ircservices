class ServiceBase
  def load_language(langfile)
    @sv_langs = {} unless @sv_langs
    @lang_map = {} unless @lang_map
    lang = File.open(@langpath+'/'+langfile+'.lang')
    langid = lang.readline.chomp.split(' ')[0].to_i
    @sv_langs[langid] = {} unless @sv_langs[langid]
    curr = @sv_langs[langid]
    last = nil
    count = 1
    lang.each_line do |line|
      if line[0,1] == "\t"
        curr[last] << line[1,line.length-1]
      else
        line.chomp!
        raise "Key Already Included #{line}" if curr.include?(line)
        @lang_map[line] = count
        count += 1
        curr[line] = ""
        last = line
      end
    end
    lang.close
    chain_language(langfile)
  end

  def reply_lang(client, mid, *args)
    langid = 0
    langid = client.nick.language if client.nick
    message = @sv_langs[langid][mid] if @sv_langs.include?(langid)
    message = @sv_langs[0][mid] unless message
    raise "Message Not Defined #{mid}" unless message
    message = message % args
    message.each_line { |l| reply_user(client, l) }
  end

  def lm(mid)
    raise "Lang Message #{mid} Does Not Exist" unless @lang_map.include?(mid)
    @lang_map[mid]
  end

  def chain_language(langfile)
  end

  def reply_user(client, message)
  end

  def is_me?(client)
    client.name.downcase == @ServiceName.downcase
  end
end
