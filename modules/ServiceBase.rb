class ServiceBase
  def load_language(langfile)
    @sv_langs = {} unless @sv_langs
    lang = File.open(langfile)
    langid = lang.readline.chomp.split(' ')[0].to_i
    @sv_langs[langid] = {} unless @sv_langs[langid]
    curr = @sv_langs[langid]
    last = nil
    lang.each_line do |line|
      if line[0,1] == "\t"
        curr[last] << line[1,line.length-1]
      else
        line.chomp!
        raise "Key Already Included #{lang}" if curr.include?(line)
        curr[line] = ""
        last = line
      end
    end
    lang.close
  end

  def reply_lang(client, mid, *args)
    langid = 0
    langid = client.nick.language if client.nick
    message = @sv_langs[langid][mid] if @sv_langs.include?(langid)
    message = @sv_langs[0][mid] unless message
    raise "Message Not Defined #{mid}" unless message
    message = message % args
    reply_user(client, message)
  end

  def reply_user(client, message)
  end
end
