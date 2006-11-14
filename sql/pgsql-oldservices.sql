-- pg_dump -F p -n oldservices -s testnet-services2
-- PostgreSQL database dump
--

SET client_encoding = 'SQL_ASCII';
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: oldservices; Type: SCHEMA; Schema: -; Owner: tjfontaine
--

CREATE SCHEMA oldservices;


SET search_path = oldservices, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: admin; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE "admin" (
    admin_id integer DEFAULT nextval(('admin_admin_id_seq'::text)::regclass) NOT NULL,
    nick_id integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices."admin" OWNER TO tjfontaine;

--
-- Name: admin_admin_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE admin_admin_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.admin_admin_id_seq OWNER TO tjfontaine;

--
-- Name: akick; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE akick (
    akick_id integer DEFAULT nextval(('akick_akick_id_seq'::text)::regclass) NOT NULL,
    channel_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT 0 NOT NULL,
    mask character varying(255),
    nick_id integer DEFAULT 0 NOT NULL,
    reason text,
    who character varying(32) DEFAULT ''::character varying NOT NULL,
    added integer DEFAULT 0 NOT NULL,
    last_used integer DEFAULT 0 NOT NULL,
    last_matched character varying(255) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.akick OWNER TO tjfontaine;

--
-- Name: akick_akick_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE akick_akick_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.akick_akick_id_seq OWNER TO tjfontaine;

--
-- Name: akill; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE akill (
    akill_id integer DEFAULT nextval(('akill_akill_id_seq'::text)::regclass) NOT NULL,
    mask character varying(255) DEFAULT ''::character varying NOT NULL,
    reason character varying(255) DEFAULT ''::character varying NOT NULL,
    who character varying(32) DEFAULT ''::character varying NOT NULL,
    "time" integer DEFAULT 0 NOT NULL,
    expires integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.akill OWNER TO tjfontaine;

--
-- Name: akill_akill_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE akill_akill_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.akill_akill_id_seq OWNER TO tjfontaine;

--
-- Name: auth; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE auth (
    auth_id integer DEFAULT nextval(('auth_auth_id_seq'::text)::regclass) NOT NULL,
    code character varying(41) DEFAULT ''::character varying NOT NULL,
    name character varying(32) DEFAULT ''::character varying NOT NULL,
    command character varying DEFAULT 'NICK_REG'::character varying NOT NULL,
    params text NOT NULL,
    create_time integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.auth OWNER TO tjfontaine;

--
-- Name: auth_auth_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE auth_auth_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.auth_auth_id_seq OWNER TO tjfontaine;

--
-- Name: auth_command_constraint_table; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE auth_command_constraint_table (
    command character varying(15) NOT NULL
);


ALTER TABLE oldservices.auth_command_constraint_table OWNER TO tjfontaine;

--
-- Name: autojoin; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE autojoin (
    autojoin_id integer DEFAULT nextval(('autojoin_autojoin_id_seq'::text)::regclass) NOT NULL,
    nick_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT 0 NOT NULL,
    channel_id integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.autojoin OWNER TO tjfontaine;

--
-- Name: autojoin_autojoin_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE autojoin_autojoin_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.autojoin_autojoin_id_seq OWNER TO tjfontaine;

--
-- Name: chanaccess; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE chanaccess (
    chanaccess_id integer DEFAULT nextval(('chanaccess_chanaccess_id_seq'::text)::regclass) NOT NULL,
    channel_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT 0 NOT NULL,
    "level" integer DEFAULT 0 NOT NULL,
    nick_id integer DEFAULT 0 NOT NULL,
    added integer DEFAULT 0 NOT NULL,
    last_modified integer DEFAULT 0 NOT NULL,
    last_used integer DEFAULT 0 NOT NULL,
    memo_read integer DEFAULT 0 NOT NULL,
    added_by character varying(32) DEFAULT ''::character varying NOT NULL,
    modified_by character varying(32) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.chanaccess OWNER TO tjfontaine;

--
-- Name: chanaccess_chanaccess_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE chanaccess_chanaccess_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.chanaccess_chanaccess_id_seq OWNER TO tjfontaine;

--
-- Name: chanlevel; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE chanlevel (
    channel_id integer DEFAULT 0 NOT NULL,
    what integer DEFAULT 0 NOT NULL,
    "level" integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.chanlevel OWNER TO tjfontaine;

--
-- Name: channel; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE channel (
    channel_id integer DEFAULT nextval(('channel_channel_id_seq'::text)::regclass) NOT NULL,
    name character varying(64) DEFAULT ''::character varying NOT NULL,
    founder integer DEFAULT 0 NOT NULL,
    successor integer DEFAULT 0 NOT NULL,
    founderpass character varying(41) DEFAULT ''::character varying NOT NULL,
    salt character varying(17) DEFAULT ''::character varying NOT NULL,
    description text NOT NULL,
    url text NOT NULL,
    email text NOT NULL,
    time_registered integer DEFAULT 0 NOT NULL,
    last_used integer DEFAULT 0 NOT NULL,
    founder_memo_read integer DEFAULT 0 NOT NULL,
    last_topic text NOT NULL,
    last_topic_setter character varying(32) DEFAULT ''::character varying NOT NULL,
    flags integer DEFAULT 0 NOT NULL,
    mlock_on integer DEFAULT 0 NOT NULL,
    mlock_off integer DEFAULT 0 NOT NULL,
    mlock_limit integer DEFAULT 0 NOT NULL,
    mlock_key character varying(255) DEFAULT ''::character varying NOT NULL,
    entry_message text NOT NULL,
    limit_offset smallint DEFAULT 0::smallint NOT NULL,
    limit_tolerance smallint DEFAULT 0::smallint NOT NULL,
    limit_period smallint DEFAULT 0::smallint NOT NULL,
    bantime smallint DEFAULT 0::smallint NOT NULL,
    last_sendpass_pass character varying(41) DEFAULT ''::character varying NOT NULL,
    last_sendpass_salt character varying(17) DEFAULT ''::character varying NOT NULL,
    floodserv_protected smallint DEFAULT 0::smallint NOT NULL,
    last_limit_time bigint,
    last_topic_time bigint,
    last_sendpass_time bigint
);


ALTER TABLE oldservices.channel OWNER TO tjfontaine;

--
-- Name: channel_channel_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE channel_channel_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.channel_channel_id_seq OWNER TO tjfontaine;

--
-- Name: chansuspend; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE chansuspend (
    chansuspend_id integer DEFAULT nextval(('chansuspend_chansuspend_id_seq'::text)::regclass) NOT NULL,
    chan_id integer DEFAULT 0 NOT NULL,
    suspend_id integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.chansuspend OWNER TO tjfontaine;

--
-- Name: chansuspend_chansuspend_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE chansuspend_chansuspend_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.chansuspend_chansuspend_id_seq OWNER TO tjfontaine;

--
-- Name: exception; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE exception (
    exception_id integer DEFAULT nextval(('exception_exception_id_seq'::text)::regclass) NOT NULL,
    mask character varying(255) DEFAULT ''::character varying NOT NULL,
    ex_limit smallint DEFAULT 0::smallint NOT NULL,
    who character varying(32) DEFAULT ''::character varying NOT NULL,
    reason character varying(255) DEFAULT ''::character varying NOT NULL,
    "time" integer DEFAULT 0 NOT NULL,
    expires integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.exception OWNER TO tjfontaine;

--
-- Name: exception_exception_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE exception_exception_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.exception_exception_id_seq OWNER TO tjfontaine;

--
-- Name: memo; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE memo (
    memo_id integer DEFAULT nextval(('memo_memo_id_seq'::text)::regclass) NOT NULL,
    "owner" character varying(64) DEFAULT ''::character varying NOT NULL,
    idx integer DEFAULT 0 NOT NULL,
    flags integer DEFAULT 0 NOT NULL,
    "time" integer DEFAULT 0 NOT NULL,
    sender character varying(32) DEFAULT '0'::character varying NOT NULL,
    text text NOT NULL,
    rot13 character varying DEFAULT 'N'::character varying NOT NULL
);


ALTER TABLE oldservices.memo OWNER TO tjfontaine;

--
-- Name: memo_memo_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE memo_memo_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.memo_memo_id_seq OWNER TO tjfontaine;

--
-- Name: memo_rot13_constraint_table; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE memo_rot13_constraint_table (
    rot13 character varying(3) NOT NULL
);


ALTER TABLE oldservices.memo_rot13_constraint_table OWNER TO tjfontaine;

--
-- Name: memoinfo; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE memoinfo (
    memoinfo_id integer DEFAULT nextval(('memoinfo_memoinfo_id_seq'::text)::regclass) NOT NULL,
    "owner" character varying(32) DEFAULT ''::character varying NOT NULL,
    max integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.memoinfo OWNER TO tjfontaine;

--
-- Name: memoinfo_memoinfo_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE memoinfo_memoinfo_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.memoinfo_memoinfo_id_seq OWNER TO tjfontaine;

--
-- Name: news; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE news (
    news_id integer DEFAULT nextval(('news_news_id_seq'::text)::regclass) NOT NULL,
    "type" smallint DEFAULT 0::smallint NOT NULL,
    text text NOT NULL,
    who character varying(32) DEFAULT ''::character varying NOT NULL,
    "time" integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.news OWNER TO tjfontaine;

--
-- Name: news_news_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE news_news_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.news_news_id_seq OWNER TO tjfontaine;

--
-- Name: nick; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE nick (
    nick_id integer DEFAULT nextval(('nick_nick_id_seq'::text)::regclass) NOT NULL,
    nick character varying(32) DEFAULT ''::character varying NOT NULL,
    pass character varying(41) DEFAULT ''::character varying NOT NULL,
    salt character varying(17) DEFAULT ''::character varying NOT NULL,
    url character varying(255) DEFAULT ''::character varying NOT NULL,
    email character varying(255) DEFAULT ''::character varying NOT NULL,
    last_usermask character varying(255) DEFAULT ''::character varying NOT NULL,
    last_realname character varying(255) DEFAULT ''::character varying NOT NULL,
    last_quit text NOT NULL,
    last_quit_time bigint DEFAULT 0 NOT NULL,
    time_registered bigint DEFAULT 0 NOT NULL,
    last_seen bigint DEFAULT 0 NOT NULL,
    status integer DEFAULT 0 NOT NULL,
    link_id integer DEFAULT 0 NOT NULL,
    flags integer DEFAULT 0 NOT NULL,
    channelmax integer DEFAULT 0 NOT NULL,
    "language" integer DEFAULT 0 NOT NULL,
    id_stamp bigint DEFAULT 0 NOT NULL,
    regainid bigint DEFAULT 0 NOT NULL,
    last_sendpass_pass character varying(41) DEFAULT ''::character varying NOT NULL,
    last_sendpass_salt character varying(17) DEFAULT ''::character varying NOT NULL,
    last_sendpass_time bigint DEFAULT 0 NOT NULL,
    cloak_string character varying(255) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.nick OWNER TO tjfontaine;

--
-- Name: nick_nick_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE nick_nick_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.nick_nick_id_seq OWNER TO tjfontaine;

--
-- Name: nickaccess; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE nickaccess (
    nickaccess_id integer DEFAULT nextval(('nickaccess_nickaccess_id_seq'::text)::regclass) NOT NULL,
    nick_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT 0 NOT NULL,
    userhost character varying(255) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.nickaccess OWNER TO tjfontaine;

--
-- Name: nickaccess_nickaccess_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE nickaccess_nickaccess_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.nickaccess_nickaccess_id_seq OWNER TO tjfontaine;

--
-- Name: nicksuspend; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE nicksuspend (
    nicksuspend_id integer DEFAULT nextval(('nicksuspend_nicksuspend_id_seq'::text)::regclass) NOT NULL,
    nick_id integer DEFAULT 0 NOT NULL,
    suspend_id integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.nicksuspend OWNER TO tjfontaine;

--
-- Name: nicksuspend_nicksuspend_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE nicksuspend_nicksuspend_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.nicksuspend_nicksuspend_id_seq OWNER TO tjfontaine;

--
-- Name: oper; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE oper (
    oper_id integer DEFAULT nextval(('oper_oper_id_seq'::text)::regclass) NOT NULL,
    nick_id integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.oper OWNER TO tjfontaine;

--
-- Name: oper_oper_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE oper_oper_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.oper_oper_id_seq OWNER TO tjfontaine;

--
-- Name: private_tmp_access; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_access (
    private_tmp_access_id integer DEFAULT nextval(('private_tmp_access_private_tmp_access_id_seq'::text)::regclass) NOT NULL,
    nick_id integer DEFAULT 0 NOT NULL,
    userhost character varying(255) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.private_tmp_access OWNER TO tjfontaine;

--
-- Name: private_tmp_access_private_tmp_access_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE private_tmp_access_private_tmp_access_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.private_tmp_access_private_tmp_access_id_seq OWNER TO tjfontaine;

--
-- Name: private_tmp_akick; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_akick (
    akick_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT nextval(('private_tmp_akick_idx_seq'::text)::regclass) NOT NULL,
    mask character varying(255),
    nick_id integer DEFAULT 0 NOT NULL,
    reason text,
    who character varying(32) DEFAULT ''::character varying NOT NULL,
    added integer DEFAULT 0 NOT NULL,
    last_used integer DEFAULT 0 NOT NULL,
    last_matched character varying(255) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.private_tmp_akick OWNER TO tjfontaine;

--
-- Name: private_tmp_akick_idx_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE private_tmp_akick_idx_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.private_tmp_akick_idx_seq OWNER TO tjfontaine;

--
-- Name: private_tmp_autojoin; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_autojoin (
    autojoin_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT nextval(('private_tmp_autojoin_idx_seq'::text)::regclass) NOT NULL,
    channel_id integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.private_tmp_autojoin OWNER TO tjfontaine;

--
-- Name: private_tmp_autojoin_idx_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE private_tmp_autojoin_idx_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.private_tmp_autojoin_idx_seq OWNER TO tjfontaine;

--
-- Name: private_tmp_chanaccess; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_chanaccess (
    chanaccess_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT nextval(('private_tmp_chanaccess_idx_seq'::text)::regclass) NOT NULL,
    "level" integer DEFAULT 0 NOT NULL,
    nick_id integer DEFAULT 0 NOT NULL,
    added integer DEFAULT 0 NOT NULL,
    last_modified integer DEFAULT 0 NOT NULL,
    last_used integer DEFAULT 0 NOT NULL,
    memo_read integer DEFAULT 0 NOT NULL,
    added_by character varying(32) DEFAULT ''::character varying NOT NULL,
    modified_by character varying(32) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.private_tmp_chanaccess OWNER TO tjfontaine;

--
-- Name: private_tmp_chanaccess_idx_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE private_tmp_chanaccess_idx_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.private_tmp_chanaccess_idx_seq OWNER TO tjfontaine;

--
-- Name: private_tmp_memo; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_memo (
    memo_id integer DEFAULT nextval(('private_tmp_memo_memo_id_seq'::text)::regclass) NOT NULL,
    "owner" character varying(64) DEFAULT ''::character varying NOT NULL,
    idx integer DEFAULT 0 NOT NULL,
    flags integer DEFAULT 0 NOT NULL,
    "time" integer DEFAULT 0 NOT NULL,
    sender character varying(32) DEFAULT '0'::character varying NOT NULL,
    text text NOT NULL,
    rot13 character varying DEFAULT 'N'::character varying NOT NULL
);


ALTER TABLE oldservices.private_tmp_memo OWNER TO tjfontaine;

--
-- Name: private_tmp_memo2; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_memo2 (
    memo_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT nextval(('private_tmp_memo2_idx_seq'::text)::regclass) NOT NULL,
    flags integer DEFAULT 0 NOT NULL,
    "time" integer DEFAULT 0 NOT NULL,
    sender character varying(32) DEFAULT '0'::character varying NOT NULL,
    text text NOT NULL,
    rot13 character varying DEFAULT 'N'::character varying NOT NULL
);


ALTER TABLE oldservices.private_tmp_memo2 OWNER TO tjfontaine;

--
-- Name: private_tmp_memo2_idx_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE private_tmp_memo2_idx_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.private_tmp_memo2_idx_seq OWNER TO tjfontaine;

--
-- Name: private_tmp_memo2_rot13_constraint_table; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_memo2_rot13_constraint_table (
    rot13 character varying(3) NOT NULL
);


ALTER TABLE oldservices.private_tmp_memo2_rot13_constraint_table OWNER TO tjfontaine;

--
-- Name: private_tmp_memo_memo_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE private_tmp_memo_memo_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.private_tmp_memo_memo_id_seq OWNER TO tjfontaine;

--
-- Name: private_tmp_memo_rot13_constraint_table; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_memo_rot13_constraint_table (
    rot13 character varying(3) NOT NULL
);


ALTER TABLE oldservices.private_tmp_memo_rot13_constraint_table OWNER TO tjfontaine;

--
-- Name: private_tmp_nickaccess2; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_nickaccess2 (
    nickaccess_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT nextval(('private_tmp_nickaccess2_idx_seq'::text)::regclass) NOT NULL,
    userhost character varying(255) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.private_tmp_nickaccess2 OWNER TO tjfontaine;

--
-- Name: private_tmp_nickaccess2_idx_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE private_tmp_nickaccess2_idx_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.private_tmp_nickaccess2_idx_seq OWNER TO tjfontaine;

--
-- Name: private_tmp_quarantine; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE private_tmp_quarantine (
    quarantine_id integer DEFAULT 0 NOT NULL,
    idx integer DEFAULT nextval(('private_tmp_quarantine_idx_seq'::text)::regclass) NOT NULL,
    regex character varying(255) DEFAULT ''::character varying NOT NULL,
    added_by character varying(255) DEFAULT ''::character varying NOT NULL,
    added_time integer DEFAULT 0 NOT NULL,
    reason character varying(255) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.private_tmp_quarantine OWNER TO tjfontaine;

--
-- Name: private_tmp_quarantine_idx_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE private_tmp_quarantine_idx_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.private_tmp_quarantine_idx_seq OWNER TO tjfontaine;

--
-- Name: quarantine; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE quarantine (
    quarantine_id integer DEFAULT nextval(('quarantine_quarantine_id_seq'::text)::regclass) NOT NULL,
    idx integer DEFAULT 0 NOT NULL,
    regex character varying(255) DEFAULT ''::character varying NOT NULL,
    added_by character varying(255) DEFAULT ''::character varying NOT NULL,
    added_time integer DEFAULT 0 NOT NULL,
    reason character varying(255) DEFAULT ''::character varying NOT NULL
);


ALTER TABLE oldservices.quarantine OWNER TO tjfontaine;

--
-- Name: quarantine_quarantine_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE quarantine_quarantine_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.quarantine_quarantine_id_seq OWNER TO tjfontaine;

--
-- Name: session; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE "session" (
    host character varying(255) DEFAULT ''::character varying NOT NULL,
    count smallint DEFAULT 0::smallint NOT NULL,
    killcount smallint DEFAULT 0::smallint NOT NULL
);


ALTER TABLE oldservices."session" OWNER TO tjfontaine;

--
-- Name: stat; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE stat (
    name character varying(255) DEFAULT ''::character varying NOT NULL,
    value text NOT NULL
);


ALTER TABLE oldservices.stat OWNER TO tjfontaine;

--
-- Name: suspend; Type: TABLE; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

CREATE TABLE suspend (
    suspend_id integer DEFAULT nextval(('suspend_suspend_id_seq'::text)::regclass) NOT NULL,
    who character varying(32) DEFAULT ''::character varying NOT NULL,
    reason text NOT NULL,
    suspended integer DEFAULT 0 NOT NULL,
    expires integer DEFAULT 0 NOT NULL
);


ALTER TABLE oldservices.suspend OWNER TO tjfontaine;

--
-- Name: suspend_suspend_id_seq; Type: SEQUENCE; Schema: oldservices; Owner: tjfontaine
--

CREATE SEQUENCE suspend_suspend_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE oldservices.suspend_suspend_id_seq OWNER TO tjfontaine;

--
-- Name: admin_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY "admin"
    ADD CONSTRAINT admin_pkey PRIMARY KEY (admin_id);


--
-- Name: akick_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY akick
    ADD CONSTRAINT akick_pkey PRIMARY KEY (akick_id);


--
-- Name: akill_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY akill
    ADD CONSTRAINT akill_pkey PRIMARY KEY (akill_id);


--
-- Name: auth_command_constraint_table_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY auth_command_constraint_table
    ADD CONSTRAINT auth_command_constraint_table_pkey PRIMARY KEY (command);


--
-- Name: auth_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY auth
    ADD CONSTRAINT auth_pkey PRIMARY KEY (auth_id);


--
-- Name: autojoin_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY autojoin
    ADD CONSTRAINT autojoin_pkey PRIMARY KEY (autojoin_id);


--
-- Name: chanaccess_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY chanaccess
    ADD CONSTRAINT chanaccess_pkey PRIMARY KEY (chanaccess_id);


--
-- Name: chanlevel_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY chanlevel
    ADD CONSTRAINT chanlevel_pkey PRIMARY KEY (channel_id, what);


--
-- Name: channel_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY channel
    ADD CONSTRAINT channel_pkey PRIMARY KEY (channel_id);


--
-- Name: chansuspend_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY chansuspend
    ADD CONSTRAINT chansuspend_pkey PRIMARY KEY (chansuspend_id);


--
-- Name: exception_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY exception
    ADD CONSTRAINT exception_pkey PRIMARY KEY (exception_id);


--
-- Name: memo_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY memo
    ADD CONSTRAINT memo_pkey PRIMARY KEY (memo_id);


--
-- Name: memo_rot13_constraint_table_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY memo_rot13_constraint_table
    ADD CONSTRAINT memo_rot13_constraint_table_pkey PRIMARY KEY (rot13);


--
-- Name: memoinfo_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY memoinfo
    ADD CONSTRAINT memoinfo_pkey PRIMARY KEY (memoinfo_id);


--
-- Name: news_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY news
    ADD CONSTRAINT news_pkey PRIMARY KEY (news_id);


--
-- Name: nick_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY nick
    ADD CONSTRAINT nick_pkey PRIMARY KEY (nick_id);


--
-- Name: nickaccess_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY nickaccess
    ADD CONSTRAINT nickaccess_pkey PRIMARY KEY (nickaccess_id);


--
-- Name: nicksuspend_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY nicksuspend
    ADD CONSTRAINT nicksuspend_pkey PRIMARY KEY (nicksuspend_id);


--
-- Name: oper_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY oper
    ADD CONSTRAINT oper_pkey PRIMARY KEY (oper_id);


--
-- Name: private_tmp_access_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_access
    ADD CONSTRAINT private_tmp_access_pkey PRIMARY KEY (private_tmp_access_id);


--
-- Name: private_tmp_akick_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_akick
    ADD CONSTRAINT private_tmp_akick_pkey PRIMARY KEY (idx);


--
-- Name: private_tmp_autojoin_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_autojoin
    ADD CONSTRAINT private_tmp_autojoin_pkey PRIMARY KEY (idx);


--
-- Name: private_tmp_chanaccess_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_chanaccess
    ADD CONSTRAINT private_tmp_chanaccess_pkey PRIMARY KEY (idx);


--
-- Name: private_tmp_memo2_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_memo2
    ADD CONSTRAINT private_tmp_memo2_pkey PRIMARY KEY (idx);


--
-- Name: private_tmp_memo2_rot13_constraint_table_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_memo2_rot13_constraint_table
    ADD CONSTRAINT private_tmp_memo2_rot13_constraint_table_pkey PRIMARY KEY (rot13);


--
-- Name: private_tmp_memo_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_memo
    ADD CONSTRAINT private_tmp_memo_pkey PRIMARY KEY (memo_id);


--
-- Name: private_tmp_memo_rot13_constraint_table_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_memo_rot13_constraint_table
    ADD CONSTRAINT private_tmp_memo_rot13_constraint_table_pkey PRIMARY KEY (rot13);


--
-- Name: private_tmp_nickaccess2_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_nickaccess2
    ADD CONSTRAINT private_tmp_nickaccess2_pkey PRIMARY KEY (idx);


--
-- Name: private_tmp_quarantine_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY private_tmp_quarantine
    ADD CONSTRAINT private_tmp_quarantine_pkey PRIMARY KEY (idx);


--
-- Name: quarantine_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY quarantine
    ADD CONSTRAINT quarantine_pkey PRIMARY KEY (quarantine_id);


--
-- Name: session_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY "session"
    ADD CONSTRAINT session_pkey PRIMARY KEY (host);


--
-- Name: stat_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY stat
    ADD CONSTRAINT stat_pkey PRIMARY KEY (name);


--
-- Name: suspend_pkey; Type: CONSTRAINT; Schema: oldservices; Owner: tjfontaine; Tablespace: 
--

ALTER TABLE ONLY suspend
    ADD CONSTRAINT suspend_pkey PRIMARY KEY (suspend_id);


--
-- Name: auth_command_constraint; Type: FK CONSTRAINT; Schema: oldservices; Owner: tjfontaine
--

ALTER TABLE ONLY auth
    ADD CONSTRAINT auth_command_constraint FOREIGN KEY (command) REFERENCES auth_command_constraint_table(command);


--
-- Name: memo_rot13_constraint; Type: FK CONSTRAINT; Schema: oldservices; Owner: tjfontaine
--

ALTER TABLE ONLY memo
    ADD CONSTRAINT memo_rot13_constraint FOREIGN KEY (rot13) REFERENCES memo_rot13_constraint_table(rot13);


--
-- Name: private_tmp_memo2_rot13_constraint; Type: FK CONSTRAINT; Schema: oldservices; Owner: tjfontaine
--

ALTER TABLE ONLY private_tmp_memo2
    ADD CONSTRAINT private_tmp_memo2_rot13_constraint FOREIGN KEY (rot13) REFERENCES private_tmp_memo2_rot13_constraint_table(rot13);


--
-- Name: private_tmp_memo_rot13_constraint; Type: FK CONSTRAINT; Schema: oldservices; Owner: tjfontaine
--

ALTER TABLE ONLY private_tmp_memo
    ADD CONSTRAINT private_tmp_memo_rot13_constraint FOREIGN KEY (rot13) REFERENCES private_tmp_memo_rot13_constraint_table(rot13);


--
-- PostgreSQL database dump complete
--

